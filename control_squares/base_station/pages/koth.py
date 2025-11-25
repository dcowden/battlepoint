import asyncio
from nicegui import ui, app
from sound_bus import attach_sound_opt_in
from multimode_app_static import *
from pages.overlays import install_winner_overlay
from http_client import get_session, close_session

@ui.page('/koth')
async def koth_game_ui():
    """KOTH game interface"""
    ui.colors(primary='#1976D2')

    ui.add_head_html(KOTH_HEAD_HTML)

    # Winner overlay for this client/page
    overlay = install_winner_overlay('koth')
    open_winner = overlay['open_winner']

    with ui.element('div').classes('bp-root'):
        # TOP BAR
        with ui.element('div').classes('bp-topbar'):
            with ui.element('div').classes('bp-topbar-left'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('KOTH MODE').classes('text-white text-lg font-bold')

            with ui.element('div').classes('bp-topbar-center'):
                blue_toggle = ui.toggle(['B OFF', 'B ON'], value='B OFF').props(
                    'dense push toggle-color=blue color=grey-8 text-color=white'
                )
                red_toggle = ui.toggle(['R OFF', 'R ON'], value='R OFF').props(
                    'dense push toggle-color=red color=grey-8 text-color=white'
                )
                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

            with ui.element('div').classes('bp-topbar-right'):
                attach_sound_opt_in()
                ui.button(
                    'Settings', on_click=lambda: ui.navigate.to('/settings?mode=koth')
                ).props('flat color=white')
                ui.button(
                    'Debug', on_click=lambda: ui.navigate.to('/debug')
                ).props('flat color=white')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main'):
            with ui.element('div').classes('bp-side'):
                ui.label('BLUE').classes('text-8xl font-bold text-blue-400')
                with ui.element('div').classes('bp-vert-shell'):
                    red_fill = ui.element('div').classes('bp-vert-fill')

            with ui.element('div').classes('bp-center'):
                game_clock = ui.label('0:00').classes('bp-clock')
                with ui.element('div').classes('bp-capture-shell'):
                    capture_fill = ui.element('div').classes('bp-capture-fill')
                    capture_mult_label = ui.label('').classes('bp-capture-mult')

                with ui.element('div').classes('bp-status-row'):
                    status_label = ui.label('Waiting...')
                    cp_owner = ui.label('Owner: ---')
                    cp_capturing = ui.label('Capturing: ---')
                    cp_contested = ui.label('Contested: False')

            with ui.element('div').classes('bp-side'):
                ui.label('RED').classes('text-8xl font-bold text-red-400')
                with ui.element('div').classes('bp-vert-shell'):
                    blue_fill = ui.element('div').classes('bp-vert-fill')

        # BOTTOM OWNERSHIP BAR
        with ui.element('div').classes('bp-bottom'):
            with ui.element('div').style('position: relative; width: 100%; height: 100%;'):
                with ui.element('div').classes('bp-horiz-shell'):
                    horiz_red = ui.element('div').classes('bp-horiz-red')

    # Event handlers
    async def start_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/koth/start')

    async def stop_game():
        s = await get_session()
        await s.post('http://localhost:8080/api/koth/stop')

    start_btn.on('click', start_game)
    stop_btn.on('click', stop_game)

    # Manual toggles (KOTH-specific)
    syncing = {'flag': False}

    async def on_red_toggle(_e):
        if syncing['flag']:
            return
        s = await get_session()
        new_val = (red_toggle.value == 'R ON')
        await s.post(f'http://localhost:8080/api/koth/manual/red/{str(new_val).lower()}')

    async def on_blue_toggle(_e):
        if syncing['flag']:
            return
        s = await get_session()
        new_val = (blue_toggle.value == 'B ON')
        await s.post(f'http://localhost:8080/api/koth/manual/blu/{str(new_val).lower()}')

    red_toggle.on('update:model-value', on_red_toggle)
    blue_toggle.on('update:model-value', on_blue_toggle)

    # UI update loop
    async def update_ui():
        try:
            while True:
                try:
                    s = await get_session()
                    async with s.get('http://localhost:8080/api/koth/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                running = state.get('running', False)
                game_id = state.get('game_id', 0)
                winner = state.get('winner')

                storage = app.storage.user
                LAST_HANDLED_KEY = 'bp_last_ended_game_koth'

                # Winner overlay
                # Track last phase + last game id on this page instance
                # - Reset when a new game starts.
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

                # Update visibility
                if phase in ('running', 'countdown') or running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Update clock
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
                        status_label.set_text(f"Winner: {state['winner']}")
                    else:
                        status_label.set_text('Waiting...')

                # Update meters
                meters = state.get('meters', {})

                m1 = meters.get('timer1')
                if m1:
                    red_fill.style(
                        f'height: {m1.get("percent", 0)}%; background: {m1.get("fg", "#0000FF")};'
                    )

                m2 = meters.get('timer2')
                if m2:
                    blue_fill.style(
                        f'height: {m2.get("percent", 0)}%; background: {m2.get("fg", "#FF0000")};'
                    )

                m_cap = meters.get('capture')
                if m_cap:
                    capture_fill.style(
                        f'width: {m_cap.get("percent", 0)}%; background: {m_cap.get("fg", "#6600ff")};'
                    )

                m_owner = meters.get('owner')
                if m_owner:
                    pct = m_owner.get("percent", 50)
                    fg = m_owner.get("fg") or "#FF0000"
                    if fg.lower() in ("#000000", "black"):
                        pct = 50
                        fg = "#000000"
                    horiz_red.style(f'width: {pct}%; background: {fg};')

                # Update CP info
                cp = state.get('control_point', {})
                cp_owner.set_text(f"Owner: {cp.get('owner', '---')}")
                cp_capturing.set_text(f"Capturing: {cp.get('capturing', '---')}")
                cp_contested.set_text(f"Contested: {cp.get('contested', False)}")

                mult = int(cp.get('capture_multiplier', 0) or 0)
                capture_mult_label.set_text(f"x{mult}" if mult > 0 else '')

                await asyncio.sleep(0.1)
        finally:
            await close_session()

    task = asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _: task.cancel())
