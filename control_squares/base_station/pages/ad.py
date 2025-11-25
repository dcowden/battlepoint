import asyncio
import sys
from typing import Any

from nicegui import ui, app, Client
from sound_bus import browser_sound_bus, attach_sound_opt_in, BrowserSoundBus, CompositeSoundSystem
from multimode_app_static import *

import aiohttp
from http_client import get_session, close_session
from pages.overlays import install_winner_overlay


@ui.page('/ad')
async def ad_game_ui():
    """AD game interface (1–3 control points, BLUE attacks, RED defends)."""
    ui.colors(primary='#1976D2')

    # Reuse the same CSS as 3CP for layout
    ui.add_head_html(THREECP_HEAD_HTML)

    # Winner overlay for this client/page
    overlay = install_winner_overlay('ad')
    open_winner = overlay['open_winner']

    # Will hold which CP indices are actually used (0,1,2 for CP1,2,3)
    used_indices = {'indices': [0]}   # default: at least CP1
    config_loaded = {'done': False}

    with ui.element('div').classes('bp-root'):
        with ui.element('div').classes('bp-topbar flex justify-between items-center px-4'):
            attach_sound_opt_in()
            # LEFT SIDE
            with ui.row().classes('gap-2 items-center'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('AD MODE').classes('text-white text-lg font-bold')

            # RIGHT SIDE
            with ui.row().classes('gap-3 items-center'):
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

                ui.button(
                    'Settings',
                    on_click=lambda: ui.navigate.to('/settings?mode=ad'),
                ).props('flat color=white')

                ui.button(
                    'Debug',
                    on_click=lambda: ui.navigate.to('/debug'),
                ).props('flat color=white')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main-3cp'):
            game_clock = ui.label('10:00').classes('bp-clock-3cp')
            status_label = ui.label('Waiting...').classes('bp-status-3cp')

            # UP TO THREE CONTROL POINTS (1, 2, 3)
            with ui.element('div').classes('bp-points-container'):
                cp_elements: list[dict[str, Any]] = []
                for i, label in enumerate(['1', '2', '3']):
                    with ui.element('div').classes('bp-cp-column') as col:
                        circle = ui.element('div').classes('bp-cp-circle')
                        circle.style('color: #fff;')
                        with circle:
                            ui.label(label)

                        with ui.element('div').classes('bp-cp-bar'):
                            bar_fill = ui.element('div').classes('bp-cp-bar-fill')

                        with ui.element('div').classes('bp-cp-toggles'):
                            red_tog = ui.toggle(['R OFF', 'R ON'], value='R OFF').props(
                                'dense push toggle-color=red color=grey-8 text-color=white'
                            )
                            blu_tog = ui.toggle(['B OFF', 'B ON'], value='B OFF').props(
                                'dense push toggle-color=blue color=grey-8 text-color=white'
                            )

                        cp_elements.append(
                            {
                                'column': col,
                                'circle': circle,
                                'bar_fill': bar_fill,
                                'red_toggle': red_tog,
                                'blu_toggle': blu_tog,
                            }
                        )

    # ---- Load AD config once to know which CPs are used ----
    async def load_ad_config():
        try:
            s = await get_session()
            async with s.get('http://localhost:8080/api/ad/settings/load') as resp:
                data = await resp.json()
            mapping = data.get('control_square_mapping', {}) if isinstance(data, dict) else {}

            cp_used = [
                bool(mapping.get('cp_1_squares')),
                bool(mapping.get('cp_2_squares')),
                bool(mapping.get('cp_3_squares')),
            ]

            # If config is missing or totally empty, fall back to CP1 only
            if not any(cp_used):
                cp_used = [True, False, False]

            indices = [i for i, used in enumerate(cp_used) if used]
            used_indices['indices'] = indices

            # Set column visibility based on config
            for i, el in enumerate(cp_elements):
                el['column'].set_visibility(i in indices)

            print(f"[AD UI] loaded config, used_indices = {used_indices['indices']}")
        except Exception as e:
            print(f"[AD UI] load_ad_config error: {e}")
            # Fallback: at least show CP1
            used_indices['indices'] = [0]
            for i, el in enumerate(cp_elements):
                el['column'].set_visibility(i == 0)
        finally:
            config_loaded['done'] = True

    # Kick off config load once after mount
    ui.timer(0.1, load_ad_config, once=True)

    # Event handlers
    async def start_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/ad/start')

    async def stop_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/ad/stop')

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    syncing = {'flag': False}

    def make_toggle_handler(cp_idx: int, team: str):
        async def handler(e):
            if syncing['flag']:
                return

            new_val = (e.sender.value or '').strip()
            on = new_val.endswith('ON')

            print(f"[DEBUG] AD UI toggle cp={cp_idx} team={team} new_value={new_val!r} on={on}")

            s = await get_session()

            # enable manual mode
            try:
                resp_mode = await s.post('http://localhost:8080/api/ad/manual/mode/true')
                print(f"[DEBUG] AD UI toggle: manual mode ON -> {resp_mode.status}")
            except Exception as ex:
                print(f"[DEBUG] AD UI toggle: error enabling manual mode: {ex}")

            # SET this CP/team state explicitly
            try:
                resp = await s.post(
                    f'http://localhost:8080/api/ad/manual/{cp_idx}/{team}/{str(on).lower()}'
                )
                txt = await resp.text()
                print(
                    f"[DEBUG] AD UI toggle: POST /manual/{cp_idx}/{team}/{on} -> "
                    f"{resp.status} {txt}"
                )
            except Exception as ex:
                print(f"[DEBUG] AD UI toggle: error posting set: {ex}")

        return handler

    for i in range(3):
        cp_elements[i]['red_toggle'].on('update:model-value', make_toggle_handler(i, 'red'))
        cp_elements[i]['blu_toggle'].on('update:model-value', make_toggle_handler(i, 'blu'))

    # UI update loop
    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get('http://localhost:8080/api/ad/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)
                winner = state.get('winner')
                cps = state.get('control_points', []) or []

                # Winner overlay logic (same as 3CP, but now with elapsed time if available)
                if not hasattr(update_ui, "_last_phase"):
                    update_ui._last_phase = phase
                    update_ui._winner_shown = False
                else:
                    prev_phase = update_ui._last_phase

                    # New game starting: clear winner flag
                    if prev_phase in ('idle', 'ended') and phase in ('countdown', 'running'):
                        update_ui._winner_shown = False

                    # Game just ended: show overlay once
                    if (
                        not getattr(update_ui, "_winner_shown", False)
                        and prev_phase in ('running', 'countdown')
                        and phase == 'ended'
                        and winner
                    ):
                        # Try to get elapsed time from backend state if provided
                        elapsed_seconds = state.get('elapsed_seconds')

                        # Fallback: derive from time limit and remaining if both exist
                        if elapsed_seconds is None:
                            total = state.get('time_limit_seconds') or state.get('total_seconds')
                            rem = state.get('remaining_seconds')
                            try:
                                if total is not None and rem is not None:
                                    total_i = int(total)
                                    rem_i = int(rem)
                                    elapsed_seconds = max(0, total_i - rem_i)
                            except (TypeError, ValueError):
                                elapsed_seconds = None

                        open_winner(winner, elapsed_seconds)
                        update_ui._winner_shown = True

                    update_ui._last_phase = phase

                # Button visibility
                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Clock
                if phase == 'countdown':
                    cd = state.get('countdown_remaining', 0)
                    game_clock.set_text(f'{cd}')
                    status_label.set_text('COUNTDOWN...')
                else:
                    rem = state.get('remaining_seconds', 0)
                    game_clock.set_text(f'{rem // 60}:{rem % 60:02d}')
                    if running:
                        status_label.set_text('GAME RUNNING')
                    elif state.get('winner'):
                        status_label.set_text(f"🏆 Winner: {state['winner']}")
                    else:
                        status_label.set_text('Waiting...')

                # Map backend CP list (only used CPs) onto the correct circles 1/2/3
                indices = used_indices['indices']
                for i, el in enumerate(cp_elements):
                    # Default: hide if not used by config
                    el['column'].set_visibility(i in indices)

                # Now apply state visuals to used CPs
                for logical_idx, cp_index in enumerate(indices):
                    if logical_idx >= len(cps):
                        continue
                    cp_data = cps[logical_idx]
                    owner = cp_data.get('owner', 'NEUTRAL')

                    # Circle color
                    if owner == 'RED':
                        color = '#FF0000'
                    elif owner == 'BLU':
                        color = '#0000FF'
                    else:
                        color = '#444'

                    cp_elements[cp_index]['circle'].style(
                        f'border-color: {color}; color: {color};'
                    )

                    progress = cp_data.get('progress', 0)
                    capturing = cp_data.get('capturing', '---')

                    if capturing == 'RED':
                        bar_color = '#FF0000'
                    elif capturing == 'BLU':
                        bar_color = '#0000FF'
                    else:
                        bar_color = '#666'

                    cp_elements[cp_index]['bar_fill'].style(
                        f'width: {progress}%; background: {bar_color};'
                    )

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _: task.cancel())
