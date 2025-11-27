import asyncio
import sys
from typing import Any

from nicegui import ui, app, Client
from sound_bus import browser_sound_bus,attach_sound_opt_in,BrowserSoundBus,CompositeSoundSystem
from multimode_app_static import *
from battlepoint_game import EnhancedGameBackend,SOUND_MAP,SoundSystem
from threecp_game import ThreeCPBackend
from clock_game import ClockBackend
from ad_game import ADBackend
import aiohttp
from http_client import get_session, close_session
from pages.overlays import install_winner_overlay
from util import make_absolute_url

# ========================================================================
# 3CP GAME PAGE
# ========================================================================

@ui.page('/3cp')
async def threecp_game_ui():
    """3CP game interface with 3 control points"""
    ui.colors(primary='#1976D2')

    ui.add_head_html(THREECP_HEAD_HTML)

    # Winner overlay for this client/page
    overlay = install_winner_overlay('3cp')
    open_winner = overlay['open_winner']

    with ui.element('div').classes('bp-root'):
        with ui.element('div').classes('bp-topbar flex justify-between items-center px-4'):
            attach_sound_opt_in()
            # LEFT SIDE
            with ui.row().classes('gap-2 items-center'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('3CP MODE').classes('text-white text-lg font-bold')

            # RIGHT SIDE
            with ui.row().classes('gap-3 items-center'):
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

                ui.button(
                    'Settings',
                    on_click=lambda: ui.navigate.to('/settings?mode=3cp')
                ).props('flat color=white')

                ui.button(
                    'Debug',
                    on_click=lambda: ui.navigate.to('/debug')
                ).props('flat color=white')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main-3cp'):
            game_clock = ui.label('10:00').classes('bp-clock-3cp')
            status_label = ui.label('Waiting...').classes('bp-status-3cp')

            # THREE CONTROL POINTS
            with ui.element('div').classes('bp-points-container'):
                cp_elements: list[dict[str, Any]] = []
                for i, label in enumerate(['R', 'C', 'B']):
                    with ui.element('div').classes('bp-cp-column') as col:
                        # --- CAPTURE RING + OWNER CIRCLE (desktop version of mobile KOTH) ---
                        with ui.element('div').classes('flex justify-center items-center mb-2'):
                            with ui.element('div').classes(
                                'relative w-40 h-40 flex items-center justify-center'
                            ):
                                # Outer capture ring (PARENT)
                                capture_ring = ui.element('div').classes(
                                    'w-full h-full rounded-full flex items-center justify-center'
                                ).style(
                                    'width: 100%; '
                                    'height: 100%; '
                                    'border-radius: 9999px; '
                                    'background: #222222; '
                                    'display: flex; '
                                    'align-items: center; '
                                    'justify-content: center; '
                                )

                                # Inner ownership circle (CHILD of ring)
                                with capture_ring:
                                    owner_circle = ui.element('div').classes(
                                        'flex items-center justify-center'
                                    ).style(
                                        'width: 70%; '
                                        'height: 70%; '
                                        'border-radius: 9999px; '
                                        'border: 4px solid #111111; '
                                        'background: #000000; '
                                        'display: flex; '
                                        'align-items: center; '
                                        'justify-content: center; '
                                    )

                        # Label under the circle so it doesn't offset center
                        owner_label = ui.label(label).classes(
                            'text-4xl font-bold text-white pointer-events-none text-center'
                        )

                        # Toggles
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
                                'capture_ring': capture_ring,
                                'owner_circle': owner_circle,
                                'owner_label': owner_label,
                                'red_toggle': red_tog,
                                'blu_toggle': blu_tog,
                            }
                        )

    # Event handlers
    async def start_game():
        s = await get_session()
        await s.post(make_absolute_url('/api/3cp/start'))

    async def stop_game():
        s = await get_session()
        await s.post(make_absolute_url('/api/3cp/stop'))

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    syncing = {'flag': False}  # if you later want to push state from backend into UI

    def make_toggle_handler(cp_idx: int, team: str):
        async def handler(e):
            # optional: guard if we later sync from backend
            if syncing['flag']:
                return

            new_val = (e.sender.value or '').strip()
            on = new_val.endswith('ON')   # 'R ON' / 'B ON' -> True, else False

            print(f"[DEBUG] 3CP UI toggle cp={cp_idx} team={team} new_value={new_val!r} on={on}")

            s = await get_session()

            # enable manual mode
            try:
                resp_mode = await s.post(make_absolute_url('/api/3cp/manual/mode/true'))
                print(f"[DEBUG] 3CP UI toggle: manual mode ON -> {resp_mode.status}")
            except Exception as ex:
                print(f"[DEBUG] 3CP UI toggle: error enabling manual mode: {ex}")

            # SET this CP/team state explicitly
            try:
                resp = await s.post(
                    make_absolute_url(f'/api/3cp/manual/{cp_idx}/{team}/{str(on).lower()}')
                )
                txt = await resp.text()
                print(
                    f"[DEBUG] 3CP UI toggle: POST /manual/{cp_idx}/{team}/{on} -> "
                    f"{resp.status} {txt}"
                )
            except Exception as ex:
                print(f"[DEBUG] 3CP UI toggle: error posting set: {ex}")

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
                    async with s.get(make_absolute_url('/api/3cp/state')) as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)
                game_id = state.get('game_id', 0)
                winner = state.get('winner')

                storage = app.storage.user
                LAST_HANDLED_KEY = 'bp_last_ended_game_3cp'

                # Winner overlay
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
                        open_winner(winner)
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

                # Update each CP
                cps = state.get('control_points', [])
                owners = state.get('cp_owners', ['NEUTRAL', 'NEUTRAL', 'NEUTRAL'])

                for i in range(min(3, len(cps))):
                    cp_data = cps[i]
                    owner = owners[i] if i < len(owners) else 'NEUTRAL'
                    capturing = cp_data.get('capturing', '---')
                    progress = cp_data.get('progress', 0)

                    # --- OWNER CENTER (solid color) ---
                    if owner == 'RED':
                        owner_color = '#FF0000'
                    elif owner == 'BLU':
                        owner_color = '#0000FF'
                    else:
                        owner_color = '#444444'

                    cp_elements[i]['owner_circle'].style(
                        'width: 70%; '
                        'height: 70%; '
                        'border-radius: 9999px; '
                        'border: 4px solid #111111; '
                        f'background: {owner_color}; '
                        'display: flex; align-items: center; justify-content: center;'
                    )

                    # --- CAPTURE RING (progress around center) ---
                    if capturing == 'RED':
                        ring_color = '#FF0000'
                    elif capturing == 'BLU':
                        ring_color = '#0000FF'
                    else:
                        ring_color = '#666666'

                    pct = max(0, min(100, progress))
                    cp_elements[i]['capture_ring'].style(
                        'width: 100%; '
                        'height: 100%; '
                        'border-radius: 9999px; '
                        'display: flex; align-items: center; justify-content: center; '
                        f'background: conic-gradient({ring_color} 0 {pct}%, '
                        f'#222222 {pct}% 100%);'
                    )

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _: task.cancel())
