from nicegui import ui, app
from sound_bus import browser_sound_bus,attach_sound_opt_in
from multimode_app_static import ROOT_HEAD_HTML


@ui.page('/')
def landing_page(kiosk: str = ''):
    ui.colors(primary='#1976D2')

    ui.add_head_html(ROOT_HEAD_HTML)
    client = ui.context.client


    # One-time detection
    if kiosk == '1':
        app.storage.user['is_kiosk'] = True

    is_kiosk = app.storage.user.get('is_kiosk', False)
    print(f"[DEBUG] landing_page: kiosk_param={kiosk!r} is_kiosk={is_kiosk} ")

    # if is_kiosk:
    #     app.storage.user['browser_sound_disabled'] = True
    #     app.storage.user['bp_sound_enabled'] = False
    #     browser_sound_bus.disable_for_client(client.id)
    # else:
    #     app.storage.user['browser_sound_disabled'] = False
    #
    #     def auto_enable_sound_and_music():
    #         # mark this client as having sound enabled
    #         app.storage.user['bp_sound_enabled'] = True
    #         browser_sound_bus.enable_for_client(client.id, from_auto=True)
    #
    #         # start / resume menu music for this client
    #         browser_sound_bus.play_menu_track()
    #
    #     # run once shortly after the page is mounted
    #     ui.timer(0.1, auto_enable_sound_and_music, once=True)


    with ui.element('div').classes('bp-landing'):
        with ui.element('div').classes('bp-card'):

            # Top row: title + sound button for kiosk
            with ui.row().classes('w-full justify-between items-center mb-2'):
                ui.html('<div class="bp-title">⚔️ BattlePoint</div>', sanitize=False)

                # When sound becomes enabled on THIS page, start menu music
                attach_sound_opt_in(
                    on_enabled=lambda _client: browser_sound_bus.play_menu_track()
                )

            ui.html('<div class="bp-subtitle">Real-life Team Fortress 2 Combat</div>', sanitize=False)

            with ui.element('div').classes('bp-mode-grid'):
                # KOTH Card
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/koth')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #ff6b6b;">🏔️ KING OF THE HILL</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        '<strong>Classic KOTH:</strong> Control the single point to drain your team\'s timer. '
                        'First team to zero wins!'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('PLAY KOTH →', on_click=lambda: ui.navigate.to('/koth')).props(
                        'unelevated color=red-7'
                    ).classes('w-full')

                # 3CP Card
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/3cp')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #4ecdc4;">🎯 THREE CONTROL POINTS</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        '<strong>Linear Progression:</strong> Capture points in sequence (A→B→C). '
                        'Fight for the middle, push to victory!'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('PLAY 3CP →', on_click=lambda: ui.navigate.to('/3cp')).props(
                        'unelevated color=teal-6'
                    ).classes('w-full')

                # NEW: AD Card (Attack/Defend multi-point)
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/ad')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #9b5de5;">🛡️ ATTACK / DEFEND</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        '<strong>BLUE attacks, RED defends:</strong> BLUE must capture 1→2→3 in order '
                        'before time runs out. Control points start RED and cannot be recaptured.'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('PLAY AD →', on_click=lambda: ui.navigate.to('/ad')).props(
                        'unelevated color=deep-purple-5'
                    ).classes('w-full')


                # CLOCK-ONLY Card
                with ui.element('div').classes('bp-mode-card').on('click', lambda: ui.navigate.to('/clock')):
                    ui.html(
                        '<div class="bp-mode-title" style="color: #ffd166;">⏱️ CLOCK ONLY</div>',
                        sanitize=False,
                    )
                    ui.html(
                        '<div class="bp-mode-desc">'
                        'A simple round clock: set minutes and seconds, start the timer, '
                        'and let BattlePoint handle announcements and sounds.'
                        '</div>',
                        sanitize=False,
                    )
                    ui.button('RUN CLOCK →', on_click=lambda: ui.navigate.to('/clock')).props(
                        'unelevated color=amber-6'
                    ).classes('w-full')

            with ui.element('div').classes('bp-info'):
                ui.label('📋 Game Instructions').classes('text-xl font-bold mb-2')
                ui.html(
                    '''
                <ul style="color: #b0b0b0; line-height: 1.8;">
                    <li><strong>KOTH:</strong> One control point. Own it to drain your timer. Reach 0:00 to win.</li>
                    <li><strong>3CP:</strong> Three points (A-B-C). Capture in sequence to push forward and win.</li>
                    <li><strong>AD:</strong> BLUE (attack) captures points 1→2→3 in order while RED defends. All points start RED and cannot be recaptured.</li>
                    <li><strong>Clock:</strong> Simple round timer with audio cues; no control points.</li>
                    <li><strong>Setup:</strong> Use Bluetooth devices or manual controls (Debug page) to simulate players for KOTH/3CP.</li>
                    <li><strong>Settings:</strong> Configure capture times, game duration, and control square mappings.</li>
                </ul>
                ''',
                    sanitize=False,
                )

            with ui.row().classes('w-full justify-center gap-4 mt-4'):
                ui.button('⚙️ Settings', on_click=lambda: ui.navigate.to('/settings')).props('outline color=white')
                ui.button('🔧 Debug', on_click=lambda: ui.navigate.to('/debug')).props('outline color=white')
