# mobile_pages.py

import asyncio
from typing import Any

from nicegui import ui, app

from sound_bus import browser_sound_bus, attach_sound_opt_in
from multimode_app_static import ROOT_HEAD_HTML
from pages.overlays import install_winner_overlay
from http_client import get_session, close_session
from pages.settings import parse_squares  # for mobile settings
import aiohttp


# =====================================================================
# MOBILE LANDING / HUB
# =====================================================================

@ui.page('/mobile')
def mobile_landing_page(admin: str = ''):
    """Mobile landing: vertical cards, simple navigation."""
    ui.colors(primary='#1976D2')
    ui.add_head_html(ROOT_HEAD_HTML)

    is_admin = admin in ('1', 'true', 'TRUE', 'yes', 'y')

    with ui.element('div').classes(
        'min-h-screen w-full flex flex-col bg-black text-white'
    ):
        # TOP BAR
        with ui.row().classes(
            'w-full items-center justify-between px-4 py-3 bg-gray-900'
        ):
            ui.label('⚔️ BattlePoint').classes('text-xl font-bold')
            # sound badge / opt-in
            attach_sound_opt_in(
                on_enabled=lambda _c: browser_sound_bus.play_menu_track()
            )

        # MAIN CONTENT
        with ui.column().classes(
            'flex-1 w-full max-w-md mx-auto px-4 py-4 gap-4'
        ):
            ui.label('Game Modes').classes('text-lg font-semibold mb-1')

            # KOTH
            with ui.card().classes('w-full bg-gray-800 text-white'):
                ui.label('🏔️ King of the Hill').classes('text-lg font-bold mb-1')
                ui.label(
                    'Control the single point to drain your timer. '
                    'First team to zero wins.'
                ).classes('text-sm text-gray-300 mb-1')
                koth_status = ui.label('Checking...').classes(
                    'text-xs text-gray-400 mb-2'
                )
                ui.button(
                    'Play KOTH',
                    on_click=lambda: ui.navigate.to('/m/koth'),
                ).props('unelevated color=red-7').classes('w-full')

            # 3CP
            with ui.card().classes('w-full bg-gray-800 text-white'):
                ui.label('🎯 Three Control Points').classes(
                    'text-lg font-bold mb-1'
                )
                ui.label(
                    'Capture R → C → B in sequence. Fight for the middle and push through.'
                ).classes('text-sm text-gray-300 mb-1')
                threecp_status = ui.label('Checking...').classes(
                    'text-xs text-gray-400 mb-2'
                )
                ui.button(
                    'Play 3CP',
                    on_click=lambda: ui.navigate.to('/m/3cp'),
                ).props('unelevated color=teal-6').classes('w-full')

            # AD
            with ui.card().classes('w-full bg-gray-800 text-white'):
                ui.label('🛡️ Attack / Defend').classes(
                    'text-lg font-bold mb-1'
                )
                ui.label(
                    'BLUE attacks, RED defends. BLUE must capture 1 → 2 → 3 before time runs out.'
                ).classes('text-sm text-gray-300 mb-1')
                ad_status = ui.label('Checking...').classes(
                    'text-xs text-gray-400 mb-2'
                )
                ui.button(
                    'Play AD',
                    on_click=lambda: ui.navigate.to('/m/ad'),
                ).props('unelevated color=deep-purple-5').classes('w-full')

            # Clock
            with ui.card().classes('w-full bg-gray-800 text-white'):
                ui.label('⏱️ Clock Only').classes('text-lg font-bold mb-1')
                ui.label(
                    'Simple round timer with audio cues. No control points.'
                ).classes('text-sm text-gray-300 mb-1')
                clock_status = ui.label('Checking...').classes(
                    'text-xs text-gray-400 mb-2'
                )
                ui.button(
                    'Run Clock',
                    on_click=lambda: ui.navigate.to('/m/clock'),
                ).props('unelevated color=amber-6').classes('w-full')

            # Admin / Tools
            with ui.column().classes('w-full mt-4 gap-2'):
                ui.label('Tools').classes('text-base font-semibold')
                ui.button(
                    '⚙️ Settings',
                    on_click=lambda: ui.navigate.to('/m/settings'),
                ).props('outline color=white').classes('w-full')

                # Debug: keep track of admin flag in URL
                debug_target = (
                    '/m/debug?admin=1' if is_admin else '/m/debug'
                )
                ui.button(
                    '🔧 Debug',
                    on_click=lambda: ui.navigate.to(debug_target),
                ).props('outline color=white').classes('w-full')

        # FOOTER / SMALL HELP
        with ui.element('div').classes(
            'w-full px-4 py-3 text-xs text-gray-400 text-center bg-gray-900'
        ):
            ui.html(
                'Scan this page on your phone for a mobile HUD. '
                'Use the full-screen / HD UI on the main scoreboard display.',
                sanitize=False,
            )

    async def _check(endpoint: str, label):
        try:
            s = await get_session()
            async with s.get(endpoint) as resp:
                data = await resp.json()
            running = bool(data.get('running', False))
            phase = data.get('phase', 'idle')
            active = running or phase in ('running', 'countdown')
            if active:
                label.set_text('IN PROGRESS')
                label.classes('text-green-400', remove='text-gray-400 text-red-400')
            else:
                label.set_text('Idle')
                label.classes('text-gray-400', remove='text-green-400 text-red-400')
        except Exception:
            label.set_text('Offline')
            label.classes('text-red-400', remove='text-gray-400 text-green-400')

    async def check_running_states():
        await asyncio.gather(
            _check('http://localhost:8080/api/koth/state', koth_status),
            _check('http://localhost:8080/api/3cp/state', threecp_status),
            _check('http://localhost:8080/api/ad/state', ad_status),
            _check('http://localhost:8080/api/clock/state', clock_status),
        )

    ui.timer(0.2, check_running_states)

# =====================================================================
# MOBILE KOTH
# =====================================================================
@ui.page('/m/koth')
async def mobile_koth_game_ui():
    """Mobile KOTH UI: stacked layout, big clock, horizontal meters + circular CP indicator."""
    ui.colors(primary='#1976D2')

    overlay = install_winner_overlay('koth')
    open_winner = overlay['open_winner']

    with ui.element('div').classes(
        'min-h-screen w-full flex flex-col bg-black text-white'
    ):
        # TOP BAR
        with ui.row().classes(
            'w-full items-center justify-between px-3 py-3 bg-gray-900'
        ):
            with ui.row().classes('items-center gap-2'):
                ui.button(
                    '←',
                    on_click=lambda: ui.navigate.to('/mobile'),
                ).props('flat color=white').classes('min-w-0 px-2')
                ui.label('KOTH (Mobile)').classes('text-lg font-bold')
            attach_sound_opt_in()

        # MAIN CONTENT
        with ui.column().classes(
            'flex-1 w-full max-w-md mx-auto px-3 py-3 gap-3'
        ):
            # Clock and status
            clock_label = ui.label('0:00').classes(
                'text-5xl font-mono text-center'
            )
            status_label = ui.label('Waiting...').classes(
                'text-sm text-center text-gray-300'
            )

            # Start/Stop + manual toggles
            with ui.row().classes(
                'w-full justify-center items-center gap-2 mt-1'
            ):
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

            with ui.row().classes(
                'w-full justify-center items-center gap-2 mt-1'
            ):
                blue_toggle = ui.toggle(['B OFF', 'B ON'], value='B OFF').props(
                    'dense push toggle-color=blue color=grey-8 text-color=white'
                )
                red_toggle = ui.toggle(['R OFF', 'R ON'], value='R OFF').props(
                    'dense push toggle-color=red color=grey-8 text-color=white'
                )

            # RED TIMER – horizontal, fills from right to left
            with ui.card().classes('w-full bg-gray-900 text-red-300'):
                ui.label('RED TIMER').classes(
                    'text-sm font-semibold mb-1'
                )
                with ui.element('div').classes(
                    'w-full h-8 bg-gray-800 rounded-full overflow-hidden '
                    'flex flex-row-reverse items-stretch'
                ):
                    red_timer_fill = ui.element('div').classes(
                        'h-full bg-red-500'
                    ).style('width: 0%;')

            # BLU TIMER – horizontal, fills from right to left
            with ui.card().classes('w-full bg-gray-900 text-blue-300'):
                ui.label('BLU TIMER').classes(
                    'text-sm font-semibold mb-1'
                )
                with ui.element('div').classes(
                    'w-full h-8 bg-gray-800 rounded-full overflow-hidden '
                    'flex flex-row-reverse items-stretch'
                ):
                    blu_timer_fill = ui.element('div').classes(
                        'h-full bg-blue-500'
                    ).style('width: 0%;')

            # CONTROL POINT: circular capture (ring) + ownership (center)
            with ui.card().classes('w-full bg-gray-900 text-gray-200 mt-1'):
                ui.label('CONTROL POINT').classes(
                    'text-sm font-semibold mb-1'
                )

                with ui.element('div').classes(
                    'w-full flex justify-center items-center py-2'
                ):
                    with ui.element('div').classes(
                        'relative w-40 h-40 flex items-center justify-center'
                    ):
                        # Thick outer ring: capture progress
                        capture_ring = ui.element('div').classes(
                            'absolute inset-0'
                        ).style(
                            'width: 100%; '
                            'height: 100%; '
                            'border-radius: 9999px; '
                            'background: #222222;'
                        )

                        # Ownership circle in the middle
                        owner_circle = ui.element('div').classes(
                            'relative flex items-center justify-center'
                        ).style(
                            'width: 70%; '
                            'height: 70%; '
                            'border-radius: 9999px; '
                            'border: 4px solid #111111; '
                            'background: #000000;'
                        )

                        owner_label = ui.label('').classes(
                            'text-xs font-bold z-10 pointer-events-none'
                        )

                capture_mult_label = ui.label('').classes(
                    'text-xs text-center text-purple-300 mt-1'
                )

                # CP info text
                cp_owner = ui.label('Owner: ---').classes('text-xs')
                cp_capturing = ui.label('Capturing: ---').classes('text-xs')
                cp_contested = ui.label('Contested: False').classes('text-xs')

    # --- Handlers & update loop ---

    async def start_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/koth/start')

    async def stop_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/koth/stop')

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    syncing = {'flag': False}

    async def on_red_toggle(_e):
        if syncing['flag']:
            return
        s = await get_session()
        new_val = (red_toggle.value == 'R ON')
        await s.post(
            f'http://localhost:8080/api/koth/manual/red/{str(new_val).lower()}'
        )

    async def on_blue_toggle(_e):
        if syncing['flag']:
            return
        s = await get_session()
        new_val = (blue_toggle.value == 'B ON')
        await s.post(
            f'http://localhost:8080/api/koth/manual/blu/{str(new_val).lower()}'
        )

    red_toggle.on('update:model-value', on_red_toggle)
    blue_toggle.on('update:model-value', on_blue_toggle)

    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get(
                        'http://localhost:8080/api/koth/state'
                    ) as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)
                winner = state.get('winner')

                # Winner overlay logic
                if not hasattr(update_ui, "_last_phase"):
                    update_ui._last_phase = phase
                    update_ui._winner_shown = False
                else:
                    prev_phase = update_ui._last_phase

                    if prev_phase in ('idle', 'ended') and phase in (
                        'countdown',
                        'running',
                    ):
                        update_ui._winner_shown = False

                    if (
                        not getattr(update_ui, "_winner_shown", False)
                        and prev_phase in ('running', 'countdown')
                        and phase == 'ended'
                        and winner
                    ):
                        open_winner(winner)
                        update_ui._winner_shown = True

                    update_ui._last_phase = phase

                # Start / stop visibility
                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Clock
                if phase == 'countdown':
                    cd = state.get('countdown_remaining', 0)
                    clock_label.set_text(f'{cd}')
                    status_label.set_text('COUNTDOWN...')
                else:
                    rem = state.get('remaining_seconds', 0)
                    clock_label.set_text(f'{rem // 60}:{rem % 60:02d}')
                    if running:
                        status_label.set_text('GAME RUNNING')
                    elif state.get('winner'):
                        status_label.set_text(f"Winner: {state['winner']}")
                    else:
                        status_label.set_text('Waiting...')

                meters = state.get('meters', {})

                # NOTE: keep mapping consistent with your original code:
                # timer1 -> BLU, timer2 -> RED (default colors)
                m1 = meters.get('timer1')
                if m1:
                    pct = max(0, min(100, m1.get('percent', 0)))
                    fg = m1.get('fg', '#0000FF')
                    blu_timer_fill.style(
                        f'width: {pct}%; '
                        f'height: 100%; '
                        f'background: {fg};'
                    )

                m2 = meters.get('timer2')
                if m2:
                    pct = max(0, min(100, m2.get('percent', 0)))
                    fg = m2.get('fg', '#FF0000')
                    red_timer_fill.style(
                        f'width: {pct}%; '
                        f'height: 100%; '
                        f'background: {fg};'
                    )

                # Capture ring (outer circle)
                m_cap = meters.get('capture')
                if m_cap:
                    pct = max(0, min(100, m_cap.get('percent', 0)))
                    fg = m_cap.get('fg', '#6600ff')
                    capture_ring.style(
                        'width: 100%; '
                        'height: 100%; '
                        'border-radius: 9999px; '
                        f'background: conic-gradient({fg} 0 {pct}%, '
                        f'#222222 {pct}% 100%);'
                    )

                # Ownership center color
                m_owner = meters.get('owner')
                cp = state.get('control_point', {})
                owner_name = cp.get('owner', '---')

                if m_owner:
                    fg = m_owner.get('fg') or '#FF0000'
                    if fg.lower() in ('#000000', 'black'):
                        fg = '#333333'
                    owner_circle.style(
                        'width: 70%; '
                        'height: 70%; '
                        'border-radius: 9999px; '
                        'border: 4px solid #111111; '
                        f'background: {fg}; '
                        'display: flex; align-items: center; '
                        'justify-content: center;'
                    )

                # Center label text based on owner
                #owner_label.set_text(owner_name if owner_name != '---' else '')

                # CP info labels
                cp_owner.set_text(f"Owner: {owner_name}")
                cp_capturing.set_text(
                    f"Capturing: {cp.get('capturing', '---')}"
                )
                cp_contested.set_text(
                    f"Contested: {cp.get('contested', False)}"
                )

                mult = int(cp.get('capture_multiplier', 0) or 0)
                capture_mult_label.set_text(f'x{mult}' if mult > 0 else '')

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    clock_label.on('disconnect', lambda _: task.cancel())


# =====================================================================
# MOBILE 3CP
# =====================================================================

@ui.page('/m/3cp')
async def mobile_threecp_game_ui():
    """Mobile 3CP UI: vertical stack of CP cards."""
    ui.colors(primary='#1976D2')
    ui.add_head_html(ROOT_HEAD_HTML)

    overlay = install_winner_overlay('3cp')
    open_winner = overlay['open_winner']

    with ui.element('div').classes(
        'min-h-screen w-full flex flex-col bg-black text-white'
    ):
        # TOP BAR
        with ui.row().classes(
            'w-full items-center justify-between px-3 py-3 bg-gray-900'
        ):
            with ui.row().classes('items-center gap-2'):
                ui.button(
                    '←',
                    on_click=lambda: ui.navigate.to('/mobile'),
                ).props('flat color=white').classes('min-w-0 px-2')
                ui.label('3CP (Mobile)').classes('text-lg font-bold')
            attach_sound_opt_in()

        # MAIN
        with ui.column().classes(
            'flex-1 w-full max-w-md mx-auto px-3 py-3 gap-3'
        ):
            game_clock = ui.label('10:00').classes(
                'text-4xl font-mono text-center'
            )
            status_label = ui.label('Waiting...').classes(
                'text-sm text-center text-gray-300'
            )

            with ui.row().classes(
                'w-full justify-center items-center gap-2 mt-1'
            ):
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

            # CP cards vertical
            cp_elements: list[dict[str, Any]] = []
            labels = ['R', 'C', 'B']
            for idx, label in enumerate(labels):
                with ui.card().classes('w-full bg-gray-900 text-gray-100'):
                    with ui.row().classes(
                        'w-full items-center justify-between mb-1'
                    ):
                        ui.label(f'Control Point {label}').classes(
                            'text-sm font-semibold'
                        )
                        circle = ui.element('div').classes(
                            'w-8 h-8 rounded-full border-2 flex items-center '
                            'justify-center text-xs font-bold'
                        ).style('border-color: #444; color: #fff;')
                        with circle:
                            ui.label(label)

                    with ui.element('div').classes(
                        'w-full h-3 bg-gray-800 rounded-full overflow-hidden mb-1'
                    ):
                        bar_fill = ui.element('div').classes(
                            'h-full'
                        ).style('width: 0%; background: #666;')

                    with ui.row().classes(
                        'w-full justify-between items-center mt-1'
                    ):
                        red_tog = ui.toggle(
                            ['R OFF', 'R ON'], value='R OFF'
                        ).props(
                            'dense push toggle-color=red color=grey-8 text-color=white'
                        )
                        blu_tog = ui.toggle(
                            ['B OFF', 'B ON'], value='B OFF'
                        ).props(
                            'dense push toggle-color=blue color=grey-8 text-color=white'
                        )

                    cp_elements.append(
                        {
                            'circle': circle,
                            'bar_fill': bar_fill,
                            'red_toggle': red_tog,
                            'blu_toggle': blu_tog,
                        }
                    )

    async def start_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/3cp/start')

    async def stop_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/3cp/stop')

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    syncing = {'flag': False}

    def make_toggle_handler(cp_idx: int, team: str):
        async def handler(e):
            if syncing['flag']:
                return
            new_val = (e.sender.value or '').strip()
            on = new_val.endswith('ON')
            print(
                f"[DEBUG] 3CP MOBILE toggle cp={cp_idx} team={team} "
                f"new_value={new_val!r} on={on}"
            )

            s = await get_session()
            try:
                resp_mode = await s.post(
                    'http://localhost:8080/api/3cp/manual/mode/true'
                )
                print(
                    f"[DEBUG] 3CP MOBILE toggle: "
                    f"manual mode ON -> {resp_mode.status}"
                )
            except Exception as ex:
                print(
                    f"[DEBUG] 3CP MOBILE toggle: error enabling manual mode: {ex}"
                )

            try:
                resp = await s.post(
                    f'http://localhost:8080/api/3cp/manual/{cp_idx}/{team}/{str(on).lower()}'
                )
                txt = await resp.text()
                print(
                    f"[DEBUG] 3CP MOBILE toggle: POST /manual/{cp_idx}/{team}/{on} "
                    f"-> {resp.status} {txt}"
                )
            except Exception as ex:
                print(
                    f"[DEBUG] 3CP MOBILE toggle: error posting set: {ex}"
                )

        return handler

    for i in range(3):
        cp_elements[i]['red_toggle'].on(
            'update:model-value', make_toggle_handler(i, 'red')
        )
        cp_elements[i]['blu_toggle'].on(
            'update:model-value', make_toggle_handler(i, 'blu')
        )

    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get(
                        'http://localhost:8080/api/3cp/state'
                    ) as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)
                winner = state.get('winner')

                # Winner overlay
                if not hasattr(update_ui, "_last_phase"):
                    update_ui._last_phase = phase
                    update_ui._winner_shown = False
                else:
                    prev_phase = update_ui._last_phase
                    if prev_phase in ('idle', 'ended') and phase in (
                        'countdown',
                        'running',
                    ):
                        update_ui._winner_shown = False

                    if (
                        not getattr(update_ui, "_winner_shown", False)
                        and prev_phase in ('running', 'countdown')
                        and phase == 'ended'
                        and winner
                    ):
                        open_winner(winner)
                        update_ui._winner_shown = True

                    update_ui._last_phase = phase

                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

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

                cps = state.get('control_points', [])
                owners = state.get(
                    'cp_owners', ['NEUTRAL', 'NEUTRAL', 'NEUTRAL']
                )

                for i in range(min(3, len(cps))):
                    cp_data = cps[i]
                    owner = owners[i] if i < len(owners) else 'NEUTRAL'

                    if owner == 'RED':
                        color = '#FF0000'
                    elif owner == 'BLU':
                        color = '#0000FF'
                    else:
                        color = '#444'

                    cp_elements[i]['circle'].style(
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

                    cp_elements[i]['bar_fill'].style(
                        f'width: {progress}%; background: {bar_color};'
                    )

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _: task.cancel())


# =====================================================================
# MOBILE AD
# =====================================================================

@ui.page('/m/ad')
async def mobile_ad_game_ui():
    """Mobile AD UI: vertical stack, variable 1–3 points."""
    ui.colors(primary='#1976D2')
    ui.add_head_html(ROOT_HEAD_HTML)

    overlay = install_winner_overlay('ad')
    open_winner = overlay['open_winner']

    used_indices = {'indices': [0]}   # default to CP1
    config_loaded = {'done': False}

    with ui.element('div').classes(
        'min-h-screen w-full flex flex-col bg-black text-white'
    ):
        with ui.row().classes(
            'w-full items-center justify-between px-3 py-3 bg-gray-900'
        ):
            with ui.row().classes('items-center gap-2'):
                ui.button(
                    '←', on_click=lambda: ui.navigate.to('/mobile')
                ).props('flat color=white').classes('min-w-0 px-2')
                ui.label('AD (Mobile)').classes('text-lg font-bold')
            attach_sound_opt_in()

        with ui.column().classes(
            'flex-1 w-full max-w-md mx-auto px-3 py-3 gap-3'
        ):
            game_clock = ui.label('10:00').classes(
                'text-4xl font-mono text-center'
            )
            status_label = ui.label('Waiting...').classes(
                'text-sm text-center text-gray-300'
            )

            with ui.row().classes(
                'w-full justify-center items-center gap-2 mt-1'
            ):
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

            # Up to three CPs
            cp_elements: list[dict[str, Any]] = []
            for idx, label in enumerate(['1', '2', '3']):
                with ui.card().classes('w-full bg-gray-900 text-gray-100') as col:
                    with ui.row().classes(
                        'w-full items-center justify-between mb-1'
                    ):
                        ui.label(f'Point {label}').classes(
                            'text-sm font-semibold'
                        )
                        circle = ui.element('div').classes(
                            'w-8 h-8 rounded-full border-2 flex items-center '
                            'justify-center text-xs font-bold'
                        ).style('border-color: #444; color: #fff;')
                        with circle:
                            ui.label(label)

                    with ui.element('div').classes(
                        'w-full h-3 bg-gray-800 rounded-full overflow-hidden mb-1'
                    ):
                        bar_fill = ui.element('div').classes(
                            'h-full'
                        ).style('width: 0%; background: #666;')

                    with ui.row().classes(
                        'w-full justify-between items-center mt-1'
                    ):
                        red_tog = ui.toggle(
                            ['R OFF', 'R ON'], value='R OFF'
                        ).props(
                            'dense push toggle-color=red color=grey-8 text-color=white'
                        )
                        blu_tog = ui.toggle(
                            ['B OFF', 'B ON'], value='B OFF'
                        ).props(
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

    async def load_ad_config():
        try:
            s = await get_session()
            async with s.get(
                'http://localhost:8080/api/ad/settings/load'
            ) as resp:
                data = await resp.json()
            mapping = (
                data.get('control_square_mapping', {})
                if isinstance(data, dict)
                else {}
            )

            cp_used = [
                bool(mapping.get('cp_1_squares')),
                bool(mapping.get('cp_2_squares')),
                bool(mapping.get('cp_3_squares')),
            ]

            if not any(cp_used):
                cp_used = [True, False, False]

            indices = [i for i, used in enumerate(cp_used) if used]
            used_indices['indices'] = indices

            for i, el in enumerate(cp_elements):
                el['column'].set_visibility(i in indices)

            print(f"[AD MOBILE] loaded config, used_indices = {indices}")
        except Exception as e:
            print(f"[AD MOBILE] load_ad_config error: {e}")
            used_indices['indices'] = [0]
            for i, el in enumerate(cp_elements):
                el['column'].set_visibility(i == 0)
        finally:
            config_loaded['done'] = True

    ui.timer(0.2, load_ad_config, once=True)

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
            print(
                f"[DEBUG] AD MOBILE toggle cp={cp_idx} team={team} "
                f"new_value={new_val!r} on={on}"
            )

            s = await get_session()
            try:
                resp_mode = await s.post(
                    'http://localhost:8080/api/ad/manual/mode/true'
                )
                print(
                    f"[DEBUG] AD MOBILE toggle: manual mode ON -> "
                    f"{resp_mode.status}"
                )
            except Exception as ex:
                print(
                    f"[DEBUG] AD MOBILE toggle: error enabling manual mode: {ex}"
                )

            try:
                resp = await s.post(
                    f'http://localhost:8080/api/ad/manual/{cp_idx}/{team}/{str(on).lower()}'
                )
                txt = await resp.text()
                print(
                    f"[DEBUG] AD MOBILE toggle: POST /manual/{cp_idx}/{team}/{on} "
                    f"-> {resp.status} {txt}"
                )
            except Exception as ex:
                print(
                    f"[DEBUG] AD MOBILE toggle: error posting set: {ex}"
                )

        return handler

    for i in range(3):
        cp_elements[i]['red_toggle'].on(
            'update:model-value', make_toggle_handler(i, 'red')
        )
        cp_elements[i]['blu_toggle'].on(
            'update:model-value', make_toggle_handler(i, 'blu')
        )

    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get(
                        'http://localhost:8080/api/ad/state'
                    ) as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)
                winner = state.get('winner')
                cps = state.get('control_points', []) or []

                # Winner overlay
                if not hasattr(update_ui, "_last_phase"):
                    update_ui._last_phase = phase
                    update_ui._winner_shown = False
                else:
                    prev_phase = update_ui._last_phase
                    if prev_phase in ('idle', 'ended') and phase in (
                        'countdown',
                        'running',
                    ):
                        update_ui._winner_shown = False

                    if (
                        not getattr(update_ui, "_winner_shown", False)
                        and prev_phase in ('running', 'countdown')
                        and phase == 'ended'
                        and winner
                    ):
                        open_winner(winner)
                        update_ui._winner_shown = True

                    update_ui._last_phase = phase

                # Buttons
                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Clock / status
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

                indices = used_indices['indices']
                for i, el in enumerate(cp_elements):
                    el['column'].set_visibility(i in indices)

                for logical_idx, cp_index in enumerate(indices):
                    if logical_idx >= len(cps):
                        continue
                    cp_data = cps[logical_idx]
                    owner = cp_data.get('owner', 'NEUTRAL')

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


# =====================================================================
# MOBILE CLOCK
# =====================================================================

import asyncio
from nicegui import ui
from sound_bus import attach_sound_opt_in
from multimode_app_static import *
from http_client import get_session, close_session


import asyncio
from nicegui import ui
from sound_bus import attach_sound_opt_in
from multimode_app_static import CLOCK_HEAD_HTML
from http_client import get_session, close_session


@ui.page('/m/clock')
async def mobile_clock_game_ui():
    """Mobile-friendly clock-only interface"""
    ui.colors(primary='#1976D2')
    ui.add_head_html(CLOCK_HEAD_HTML)
    ui.add_head_html("""
    <style>
    /* --- Clock Sizing Fix --- */
    .bp-clock-face, .bp-clock {
        /* was: clamp(18rem, 50rem, 60rem) which explodes on mobile */
        font-size: clamp(4rem, 15vw, 10rem);
        line-height: 1.1;
        text-align: center;
    }

    /* --- Input Fields Now Visible --- */
    .bp-input, 
    .bp-input input,
    .q-input__control,
    .q-field__control,
    .q-field__native {
        color: white !important;
        background: rgba(255,255,255,0.05) !important;
        border: 1px solid rgba(255,255,255,0.6) !important;
        border-radius: 8px !important;
        padding: 6px 10px !important;
        font-size: 1.1rem !important;
    }

    .q-field__label {
        color: rgba(255,255,255,0.8) !important;
        font-size: 0.9rem !important;
    }

    .q-field--filled .q-field__control:before {
        border-bottom: 1px solid rgba(255,255,255,0.6) !important;
    }

    /* Force margin so inputs don’t collapse into the clock */
    .bp-clock-input-row {
        margin-top: 1.2rem;
        margin-bottom: 1.2rem;
    }

    /* Optional improvement: center container better */
    .bp-clock-settings {
        width: 100%;
        max-width: 380px;
        margin: 0 auto;
    }
    </style>
    """)

    with ui.element('div').classes('bp-root-clock'):
        # TOP BAR (mobile)
        with ui.element('div').classes('bp-topbar-clock'):
            with ui.row().classes('gap-2 items-center'):
                ui.button('←', on_click=lambda: ui.navigate.to('/mobile')).props(
                    'flat color=white'
                )
                ui.label('Clock (Mobile)').classes('text-white text-lg font-bold')
            attach_sound_opt_in()

        # MAIN CONTENT
        with ui.element('div').classes('bp-main-clock'):
            clock_label = ui.label('10:00').classes('bp-clock-face')

            with ui.column().classes('items-center gap-3'):
                ui.label('Round Time').classes('text-lg font-semibold')

                # Visible, easy-to-edit numeric inputs
                with ui.row().classes('items-end gap-3'):
                    minutes_input = ui.input(
                        label='Min',
                        value='10'
                    ).props(
                        'outlined color=white label-color=white '
                        'input-class="text-white text-center text-xl"'
                    ).style(
                        'max-width: 90px;'
                        'color: white;'
                    )

                    seconds_input = ui.input(
                        label='Sec',
                        value='0',
                    ).props(
                        'outlined color=white label-color=white '
                        'input-class="text-white text-center text-xl"'
                    ).style(
                        'max-width: 90px;'
                        'color: white;'
                    )

                with ui.row().classes('gap-3 mt-3'):
                    start_btn = ui.button('START', icon='play_arrow').props('color=green')
                    stop_btn = ui.button('STOP', icon='stop').props('color=orange')
                    stop_btn.set_visibility(False)

            status_label = ui.label('Waiting...').classes('bp-status-clock')

    # Handlers
    async def start_clock():
        try:
            mins = int((minutes_input.value or '0').strip())
        except Exception:
            mins = 0
        try:
            secs = int((seconds_input.value or '0').strip())
        except Exception:
            secs = 0

        mins = max(0, min(99, mins))
        secs = max(0, min(59, secs))

        total_seconds = max(1, mins * 60 + secs)

        s = await get_session()
        await s.post(
            'http://localhost:8080/api/clock/configure',
            json={'time_limit_seconds': total_seconds},
        )
        await s.post('http://localhost:8080/api/clock/start')

    async def stop_clock():
        s = await get_session()
        await s.post('http://localhost:8080/api/clock/stop')

    start_btn.on('click', start_clock)
    stop_btn.on('click', stop_clock)

    initialized = {'done': False}

    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get('http://localhost:8080/api/clock/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                running = bool(state.get('running', False))
                phase = state.get('phase', 'idle')
                remaining = int(state.get('remaining_seconds', 0))
                total = int(state.get('time_limit_seconds', max(remaining, 1)))

                # Initialize inputs from backend once
                if not initialized['done']:
                    base = total if phase == 'idle' else remaining
                    m0, s0 = divmod(base, 60)
                    minutes_input.value = str(m0)
                    seconds_input.value = str(s0)
                    minutes_input.update()
                    seconds_input.update()
                    initialized['done'] = True

                # Buttons
                if running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Clock
                mins = remaining // 60
                secs = remaining % 60
                clock_label.set_text(f'{mins}:{secs:02d}')

                # Status
                if phase == 'idle':
                    status_label.set_text('Waiting...')
                elif phase == 'running':
                    status_label.set_text('Clock running')
                elif phase == 'ended':
                    status_label.set_text('Time up')
                else:
                    status_label.set_text(phase)

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    clock_label.on('disconnect', lambda _: task.cancel())


# =====================================================================
# MOBILE SETTINGS (thin, mobile-first layout)
# =====================================================================

@ui.page('/m/settings')
async def mobile_settings_ui(mode: str = 'koth'):
    """Mobile wrapper over your settings API, with vertical layout."""
    ui.colors(primary='#1976D2')
    ui.add_head_html(ROOT_HEAD_HTML)

    current_mode = mode if mode in ['koth', '3cp', 'ad'] else 'koth'
    selected_label_map = {'koth': 'KOTH', '3cp': '3CP', 'ad': 'AD'}
    selected_label = selected_label_map[current_mode]

    with ui.element('div').classes(
        'min-h-screen w-full flex flex-col bg-black text-white'
    ):
        with ui.row().classes(
            'w-full items-center justify-between px-3 py-3 bg-gray-900'
        ):
            with ui.row().classes('items-center gap-2'):
                ui.button(
                    '←', on_click=lambda: ui.navigate.to('/mobile')
                ).props('flat color=white').classes('min-w-0 px-2')
                ui.label(f'{selected_label} Settings').classes(
                    'text-lg font-bold'
                )

        with ui.column().classes(
            'flex-1 w-full max-w-md mx-auto px-3 py-3 gap-3'
        ):
            with ui.tabs().classes('w-full') as mode_tabs:
                ui.tab('KOTH')
                ui.tab('3CP')
                ui.tab('AD')

            mode_tabs.set_value(selected_label)

            def on_mode_change(e):
                selected = e.sender.value
                if selected == 'KOTH':
                    new_mode = 'koth'
                elif selected == '3CP':
                    new_mode = '3cp'
                else:
                    new_mode = 'ad'
                ui.navigate.to(f'/m/settings?mode={new_mode}')

            mode_tabs.on('update:model-value', on_mode_change)

            with ui.tab_panels(mode_tabs, value=selected_label).classes(
                'w-full'
            ):
                # KOTH
                with ui.tab_panel('KOTH'):
                    koth_mode_select = ui.select(
                        ['KOTH', 'AD', 'CP'],
                        label='Game Mode',
                        value='KOTH',
                    ).classes('w-full')
                    koth_time_limit = ui.number(
                        'Time Limit (seconds)', value=60, min=10, max=600
                    ).classes('w-full')
                    koth_capture_time = ui.number(
                        'Capture Time (seconds)', value=20, min=5, max=120
                    ).classes('w-full')
                    koth_button_threshold = ui.number(
                        'Button Threshold (seconds)',
                        value=5,
                        min=1,
                        max=30,
                    ).classes('w-full')
                    koth_start_delay = ui.number(
                        'Start Delay (seconds)', value=5, min=0, max=60
                    ).classes('w-full')

                    async def save_koth():
                        mode_map = {'KOTH': 0, 'AD': 1, 'CP': 2}
                        payload = {
                            'mode': mode_map[koth_mode_select.value],
                            'time_limit_seconds': int(koth_time_limit.value),
                            'capture_seconds': int(
                                koth_capture_time.value
                            ),
                            'capture_button_threshold_seconds': int(
                                koth_button_threshold.value
                            ),
                            'start_delay_seconds': int(
                                koth_start_delay.value
                            ),
                        }
                        try:
                            async with aiohttp.ClientSession() as session:
                                async with session.post(
                                    'http://localhost:8080/api/koth/configure',
                                    json=payload,
                                ) as resp:
                                    if resp.status != 200:
                                        ui.notify(
                                            f'KOTH configure failed: {resp.status}',
                                            type='negative',
                                        )
                                        return
                                async with session.post(
                                    'http://localhost:8080/api/koth/settings/save'
                                ) as resp:
                                    if resp.status != 200:
                                        ui.notify(
                                            f'KOTH save failed: {resp.status}',
                                            type='negative',
                                        )
                                        return
                            ui.notify('KOTH settings saved!', type='positive')
                        except Exception as e:
                            ui.notify(
                                f'Error saving KOTH settings: {e}',
                                type='negative',
                            )

                    ui.button(
                        'Save KOTH Settings', on_click=save_koth
                    ).props('color=primary').classes('mt-2 w-full')

                # 3CP
                with ui.tab_panel('3CP'):
                    threecp_time_limit = ui.number(
                        'Time Limit (seconds)', value=600, min=20, max=3600
                    ).classes('w-full')
                    threecp_capture_time = ui.number(
                        'Capture Time (seconds)', value=20, min=2, max=120
                    ).classes('w-full')
                    threecp_button_threshold = ui.number(
                        'Button Threshold (seconds)',
                        value=5,
                        min=1,
                        max=30,
                    ).classes('w-full')
                    threecp_start_delay = ui.number(
                        'Start Delay (seconds)', value=5, min=0, max=60
                    ).classes('w-full')
                    threecp_time_add = ui.number(
                        'Time Added Per First Capture (seconds)',
                        value=120,
                        min=0,
                        max=600,
                    ).classes('w-full')

                    ui.separator().classes('my-2')
                    ui.label('Control Square Mappings').classes(
                        'text-sm font-semibold'
                    )
                    cp1_input = ui.input(
                        label='CP-Red Squares (IDs)',
                        placeholder='1, 2',
                        value='1',
                    ).classes('w-full')
                    cp2_input = ui.input(
                        label='CP-Center Squares (IDs)',
                        placeholder='3, 4',
                        value='3',
                    ).classes('w-full')
                    cp3_input = ui.input(
                        label='CP-Blue Squares (IDs)',
                        placeholder='5',
                        value='5',
                    ).classes('w-full')

                    async def save_3cp():
                        cp1_squares = parse_squares(cp1_input.value)
                        cp2_squares = parse_squares(cp2_input.value)
                        cp3_squares = parse_squares(cp3_input.value)

                        payload = {
                            'time_limit_seconds': int(
                                threecp_time_limit.value
                            ),
                            'capture_seconds': int(
                                threecp_capture_time.value
                            ),
                            'capture_button_threshold_seconds': int(
                                threecp_button_threshold.value
                            ),
                            'start_delay_seconds': int(
                                threecp_start_delay.value
                            ),
                            'add_time_per_first_capture_seconds': int(
                                threecp_time_add.value
                            ),
                            'control_square_mapping': {
                                'cp_1_squares': cp1_squares,
                                'cp_2_squares': cp2_squares,
                                'cp_3_squares': cp3_squares,
                            },
                        }

                        try:
                            async with aiohttp.ClientSession() as session:
                                async with session.post(
                                    'http://localhost:8080/api/3cp/settings/save',
                                    json=payload,
                                ) as resp:
                                    if resp.status != 200:
                                        ui.notify(
                                            f'3CP save failed: {resp.status}',
                                            type='negative',
                                        )
                                        return
                            ui.notify('3CP settings saved!', type='positive')
                        except Exception as e:
                            ui.notify(
                                f'Error saving 3CP settings: {e}',
                                type='negative',
                            )

                    ui.button(
                        'Save 3CP Settings', on_click=save_3cp
                    ).props('color=primary').classes('mt-2 w-full')

                # AD
                with ui.tab_panel('AD'):
                    ad_time_limit = ui.number(
                        'Time Limit (seconds)', value=600, min=5, max=3600
                    ).classes('w-full')
                    ad_capture_time = ui.number(
                        'Capture Time (seconds)', value=20, min=2, max=120
                    ).classes('w-full')
                    ad_button_threshold = ui.number(
                        'Button Threshold (seconds)',
                        value=5,
                        min=1,
                        max=30,
                    ).classes('w-full')
                    ad_start_delay = ui.number(
                        'Start Delay (seconds)', value=5, min=0, max=60
                    ).classes('w-full')
                    ad_time_add = ui.number(
                        'Time Added Per First Capture (seconds)',
                        value=120,
                        min=0,
                        max=600,
                    ).classes('w-full')

                    ui.separator().classes('my-2')
                    ui.label('Control Square Mappings').classes(
                        'text-sm font-semibold'
                    )
                    cp1_input_ad = ui.input(
                        label='CP-1 Squares (IDs)',
                        placeholder='1, 2',
                        value='1',
                    ).classes('w-full')
                    cp2_input_ad = ui.input(
                        label='CP-2 Squares (IDs)',
                        placeholder='3, 4',
                        value='',
                    ).classes('w-full')
                    cp3_input_ad = ui.input(
                        label='CP-3 Squares (IDs)',
                        placeholder='5',
                        value='',
                    ).classes('w-full')

                    async def save_ad():
                        cp1_squares = parse_squares(cp1_input_ad.value)
                        cp2_squares = parse_squares(cp2_input_ad.value)
                        cp3_squares = parse_squares(cp3_input_ad.value)

                        payload = {
                            'time_limit_seconds': int(ad_time_limit.value),
                            'capture_seconds': int(ad_capture_time.value),
                            'capture_button_threshold_seconds': int(
                                ad_button_threshold.value
                            ),
                            'start_delay_seconds': int(ad_start_delay.value),
                            'add_time_per_first_capture_seconds': int(
                                ad_time_add.value
                            ),
                            'control_square_mapping': {
                                'cp_1_squares': cp1_squares,
                                'cp_2_squares': cp2_squares,
                                'cp_3_squares': cp3_squares,
                            },
                        }

                        try:
                            async with aiohttp.ClientSession() as session:
                                async with session.post(
                                    'http://localhost:8080/api/ad/settings/save',
                                    json=payload,
                                ) as resp:
                                    if resp.status != 200:
                                        ui.notify(
                                            f'AD save failed: {resp.status}',
                                            type='negative',
                                        )
                                        return
                            ui.notify('AD settings saved!', type='positive')
                        except Exception as e:
                            ui.notify(
                                f'Error saving AD settings: {e}',
                                type='negative',
                            )

                    ui.button(
                        'Save AD Settings', on_click=save_ad
                    ).props('color=primary').classes('mt-2 w-full')

    # Load initial settings for the selected mode
    async def load_initial_settings():
        try:
            async with aiohttp.ClientSession() as session:
                if current_mode == 'koth':
                    async with session.get(
                        'http://localhost:8080/api/koth/settings/load'
                    ) as resp:
                        data = await resp.json()
                    if data.get('status') != 'not_found':
                        mode_rev = {0: 'KOTH', 1: 'AD', 2: 'CP'}
                        koth_mode_select.value = mode_rev.get(
                            data.get('mode', 0), 'KOTH'
                        )
                        koth_time_limit.value = data.get(
                            'time_limit_seconds', 60
                        )
                        koth_capture_time.value = data.get(
                            'capture_seconds', 20
                        )
                        koth_button_threshold.value = data.get(
                            'capture_button_threshold_seconds', 5
                        )
                        koth_start_delay.value = data.get(
                            'start_delay_seconds', 5
                        )
                        for c in (
                            koth_mode_select,
                            koth_time_limit,
                            koth_capture_time,
                            koth_button_threshold,
                            koth_start_delay,
                        ):
                            c.update()

                elif current_mode == '3cp':
                    async with session.get(
                        'http://localhost:8080/api/3cp/settings/load'
                    ) as resp:
                        data = await resp.json()
                    if data.get('status') != 'not_found':
                        threecp_time_limit.value = data.get(
                            'time_limit_seconds', 600
                        )
                        threecp_capture_time.value = data.get(
                            'capture_seconds', 20
                        )
                        threecp_button_threshold.value = data.get(
                            'capture_button_threshold_seconds', 5
                        )
                        threecp_start_delay.value = data.get(
                            'start_delay_seconds', 5
                        )
                        threecp_time_add.value = data.get(
                            'add_time_per_first_capture_seconds', 120
                        )
                        mapping = data.get('control_square_mapping', {})
                        cp1_input.value = ','.join(
                            map(str, mapping.get('cp_1_squares', [1]))
                        )
                        cp2_input.value = ','.join(
                            map(str, mapping.get('cp_2_squares', [3]))
                        )
                        cp3_input.value = ','.join(
                            map(str, mapping.get('cp_3_squares', [5]))
                        )
                        for c in (
                            threecp_time_limit,
                            threecp_capture_time,
                            threecp_button_threshold,
                            threecp_start_delay,
                            threecp_time_add,
                            cp1_input,
                            cp2_input,
                            cp3_input,
                        ):
                            c.update()

                else:
                    async with session.get(
                        'http://localhost:8080/api/ad/settings/load'
                    ) as resp:
                        data = await resp.json()
                    if data.get('status') != 'not_found':
                        ad_time_limit.value = data.get(
                            'time_limit_seconds', 600
                        )
                        ad_capture_time.value = data.get(
                            'capture_seconds', 20
                        )
                        ad_button_threshold.value = data.get(
                            'capture_button_threshold_seconds', 5
                        )
                        ad_start_delay.value = data.get(
                            'start_delay_seconds', 5
                        )
                        ad_time_add.value = data.get(
                            'add_time_per_first_capture_seconds', 120
                        )
                        mapping = data.get('control_square_mapping', {})
                        cp1_input_ad.value = ','.join(
                            map(str, mapping.get('cp_1_squares', []))
                        )
                        cp2_input_ad.value = ','.join(
                            map(str, mapping.get('cp_2_squares', []))
                        )
                        cp3_input_ad.value = ','.join(
                            map(str, mapping.get('cp_3_squares', []))
                        )
                        for c in (
                            ad_time_limit,
                            ad_capture_time,
                            ad_button_threshold,
                            ad_start_delay,
                            ad_time_add,
                            cp1_input_ad,
                            cp2_input_ad,
                            cp3_input_ad,
                        ):
                            c.update()
        except Exception as e:
            print(f"[MOBILE SETTINGS] error loading initial settings: {e}")

    await load_initial_settings()


# =====================================================================
# MOBILE DEBUG (simplified wrapper)
# =====================================================================

@ui.page('/m/debug')
def mobile_debug_ui(admin: str = ''):
    """Slimmed-down, mobile-friendly debug view using the same backend APIs."""
    ui.colors(primary='#1976D2')
    ui.add_head_html(ROOT_HEAD_HTML)

    came_from_mobile = True  # kept for parity if you want to extend later

    with ui.element('div').classes(
        'min-h-screen w-full flex flex-col bg-black text-white'
    ):
        with ui.row().classes(
            'w-full items-center justify-between px-3 py-3 bg-gray-900'
        ):
            with ui.row().classes('items-center gap-2'):
                def go_back():
                    target = f'/mobile?admin={admin}' if admin else '/mobile'
                    ui.navigate.to(target)

                ui.button('←', on_click=go_back).props(
                    'flat color=white'
                ).classes('min-w-0 px-2')
                ui.label('Debug (Mobile)').classes('text-lg font-bold')

        with ui.column().classes(
            'flex-1 w-full max-w-md mx-auto px-3 py-3 gap-3'
        ):
            # Manual control
            with ui.card().classes('w-full bg-gray-900'):
                manual_switch = ui.switch(
                    'Manual Proximity Control'
                ).classes('text-sm')
                manual_status = ui.label('').classes(
                    'text-xs text-gray-300 mt-1'
                )

            # BLE status
            with ui.card().classes('w-full bg-gray-900'):
                ui.label('Bluetooth Scanner').classes(
                    'text-sm font-semibold mb-1'
                )
                ble_status = ui.label('Status: Not scanning').classes(
                    'text-xs mb-1'
                )
                device_count_label = ui.label('Tracking 0 device(s)').classes(
                    'text-xs mb-2'
                )
                with ui.row().classes(
                    'w-full justify-between items-center gap-2'
                ):
                    start_scan_btn = ui.button('Start Scan').props(
                        'color=green'
                    )
                    stop_scan_btn = ui.button('Stop').props('color=red')
                    stop_scan_btn.set_visibility(False)

            # Event log (tail only)
            with ui.card().classes('w-full bg-gray-900'):
                ui.label('Recent Events').classes(
                    'text-sm font-semibold mb-1'
                )
                event_box = ui.column().classes(
                    'w-full bg-black rounded p-2 gap-1'
                ).style('max-height: 220px; overflow-y: auto;')
                event_box_id = f"bp-event-box-mobile-{event_box.id}"
                event_box.props(f'id={event_box_id}')
                event_count_label = ui.label('0 events').classes(
                    'text-xs text-gray-400 mt-1'
                )

            # Game state glimpse
            with ui.card().classes('w-full bg-gray-900'):
                ui.label('Game State (KOTH)').classes(
                    'text-sm font-semibold mb-1'
                )
                state_display = ui.json_editor({}).classes('w-full')

    ui.run_javascript(
        f"""
    (function() {{
        const el = document.getElementById('{event_box_id}');
        if (!el) return;
        el.addEventListener('scroll', () => {{
            el.dataset.userScroll = (el.scrollTop > 5) ? '1' : '';
        }});
    }})();
    """
    )

    last_events: list[str] = []

    async def sync_manual_from_backend():
        try:
            s = await get_session()
            async with s.get(
                'http://localhost:8080/api/manual/state'
            ) as resp:
                st = await resp.json()
            enabled = bool(st.get('manual_control'))
            manual_switch.value = enabled
            manual_switch.update()
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'}; "
                "BLE scanning still runs."
            )
        except Exception as e:
            print(f"[mobile debug] manual sync failed: {e}")

    async def on_manual_change(_e):
        try:
            s = await get_session()
            enabled = bool(manual_switch.value)
            await s.post(
                f'http://localhost:8080/api/manual/mode/{str(enabled).lower()}'
            )
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'}; "
                "BLE scanning still runs."
            )
            ui.notify(
                f"Manual control {'enabled' if enabled else 'disabled'}",
                type='info',
            )
        except Exception as ex:
            print(f"[mobile debug] manual toggle failed: {ex}")

    manual_switch.on('update:model-value', on_manual_change)

    async def start_ble_scan():
        s = await get_session()
        await s.post('http://localhost:8080/api/bluetooth/start')
        start_scan_btn.set_visibility(False)
        stop_scan_btn.set_visibility(True)
        ble_status.set_text('Status: Scanning...')
        ui.notify('BLE scanning started', type='positive')

    async def stop_ble_scan():
        s = await get_session()
        await s.post('http://localhost:8080/api/bluetooth/stop')
        start_scan_btn.set_visibility(True)
        stop_scan_btn.set_visibility(False)
        ble_status.set_text('Status: Not scanning')
        ui.notify('BLE scanning stopped', type='info')

    start_scan_btn.on('click', start_ble_scan)
    stop_scan_btn.on('click', stop_ble_scan)

    async def update_debug_once():
        if state_display.client is None:
            return

        s = await get_session()
        try:
            # game state
            async with s.get('http://localhost:8080/api/koth/state') as resp:
                state = await resp.json()
                state_display.value = state
                state_display.update()

                events = state.get('events', [])
                event_count_label.set_text(f'{len(events)} event(s)')

                if events != last_events:
                    event_box.clear()
                    with event_box:
                        # only show last ~20 lines for mobile
                        for line in reversed(events[-20:]):
                            ui.label(line).classes(
                                'text-xs text-gray-100'
                            )
                    last_events[:] = events

                    await ui.run_javascript(
                        f"""
                    (function(){{
                        const el = document.getElementById('{event_box_id}');
                        if (!el) return;
                        if (!el.dataset.userScroll) {{
                            el.scrollTop = 0;
                        }}
                    }})();
                    """
                    )

            await sync_manual_from_backend()

            async with s.get(
                'http://localhost:8080/api/bluetooth/devices'
            ) as resp:
                ble_data = await resp.json()

                if ble_data.get('scanning'):
                    ble_status.set_text('Status: Scanning...')
                    start_scan_btn.set_visibility(False)
                    stop_scan_btn.set_visibility(True)
                else:
                    ble_status.set_text('Status: Not scanning')
                    start_scan_btn.set_visibility(True)
                    stop_scan_btn.set_visibility(False)

                device_count = ble_data.get('device_count', 0)
                device_count_label.set_text(
                    f'Tracking {device_count} device(s)'
                )

        except Exception as e:
            print(f"[mobile debug] update error: {e}")

    ui.timer(0.5, update_debug_once, once=False)
