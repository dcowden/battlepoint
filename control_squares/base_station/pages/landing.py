from nicegui import ui, app
import asyncio
from sound_bus import browser_sound_bus, attach_sound_opt_in
from multimode_app_static import ROOT_HEAD_HTML
from http_client import get_session
from util import make_absolute_url
from pages.qr import QRInfo   # <-- use the same IP + URL logic as /qr page

QR_SIZE = 300  # QR size in pixels (width and height), match pages/qr.py


@ui.page('/')
def landing_page(kiosk: str = ''):
    ui.colors(primary='#1976D2')
    ui.add_head_html(ROOT_HEAD_HTML)

    # Kiosk detection
    if kiosk == '1':
        app.storage.user['is_kiosk'] = True
    is_kiosk = app.storage.user.get('is_kiosk', False)
    print(f"[DEBUG] landing_page: kiosk_param={kiosk!r} is_kiosk={is_kiosk} ")

    with ui.element('div').classes('bp-landing'):
        with ui.element('div').classes('bp-card'):

            # -----------------------------
            # Title Row
            # -----------------------------
            with ui.row().classes('w-full justify-between items-center mb-2'):
                ui.html('<div class="bp-title">⚔️ BattlePoint</div>', sanitize=False)

                attach_sound_opt_in(
                    on_enabled=lambda _client: browser_sound_bus.play_menu_track()
                )

            # NOTE: subtitle removed for vertical space
            # ui.html('<div class="bp-subtitle">Real-life Team Fortress 2 Combat</div>', sanitize=False)

            # -----------------------------
            # Mode Grid
            # -----------------------------
            with ui.element('div').classes('bp-mode-grid'):

                # ---------------- KOTH ----------------
                with ui.element('div').classes('bp-mode-card').on(
                    'click', lambda: ui.navigate.to('/koth')
                ):
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
                    koth_status = ui.label('Checking...').classes(
                        'text-xs text-gray-400 mt-1'
                    )
                    ui.button(
                        'PLAY KOTH →', on_click=lambda: ui.navigate.to('/koth')
                    ).props('unelevated color=red-7').classes('w-full')

                # ---------------- 3CP ----------------
                with ui.element('div').classes('bp-mode-card').on(
                    'click', lambda: ui.navigate.to('/3cp')
                ):
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
                    threecp_status = ui.label('Checking...').classes(
                        'text-xs text-gray-400 mt-1'
                    )
                    ui.button(
                        'PLAY 3CP →', on_click=lambda: ui.navigate.to('/3cp')
                    ).props('unelevated color=teal-6').classes('w-full')

                # ---------------- AD ----------------
                with ui.element('div').classes('bp-mode-card').on(
                    'click', lambda: ui.navigate.to('/ad')
                ):
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
                    ad_status = ui.label('Checking...').classes(
                        'text-xs text-gray-400 mt-1'
                    )
                    ui.button(
                        'PLAY AD →', on_click=lambda: ui.navigate.to('/ad')
                    ).props('unelevated color=deep-purple-5').classes('w-full')

                # ---------------- CLOCK ----------------
                with ui.element('div').classes('bp-mode-card').on(
                    'click', lambda: ui.navigate.to('/clock')
                ):
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
                    clock_status = ui.label('Checking...').classes(
                        'text-xs text-gray-400 mt-1'
                    )
                    ui.button(
                        'RUN CLOCK →', on_click=lambda: ui.navigate.to('/clock')
                    ).props('unelevated color=amber-6').classes('w-full')

            # ---------------------------------------------------------------------
            # QR CODE STRIP (white background, values underneath, using QRInfo.app_url)
            # ---------------------------------------------------------------------
            with ui.element('div').classes('w-full mt-6'):

                with ui.row().classes('w-full justify-center gap-32'):


                    # ----- WiFi 1 -----
                    with ui.column().classes('items-center gap-2'):
                        ui.label('1 WiFi: bluedirt-mobile').classes(
                            'text-2xl font-bold text-white'
                        )

                        with ui.element('div').classes('bg-white p-2 rounded shadow-md'):
                            ui.image('/qr/wifi1.svg').style(
                                f'width: {QR_SIZE}px; height: {QR_SIZE}px;'
                            )

                        ui.label('Password:').classes('text-2xl font-bold text-white')
                        ui.label('bluedirt4281!').classes('text-2xl text-white')

                    # ----- APP URL -----
                    with ui.column().classes('items-center gap-2'):
                        ui.label('2. Join Game').classes('text-2xl font-bold text-white')

                        with ui.element('div').classes('bg-white p-2 rounded shadow-md'):
                            ui.image('/qr/app.svg').style(
                                f'width: {QR_SIZE}px; height: {QR_SIZE}px;'
                            )

                        ui.label('URL:').classes('text-2xl font-bold text-white')
                        # Use the same IP-based URL as QRInfo.app_url
                        ui.label(QRInfo.app_url).classes(
                            'text-xl text-white break-all text-center max-w-xs'
                        )

                    # # ----- WiFi 2 -----
                    # with ui.column().classes('items-center gap-2'):
                    #     ui.label('WiFi: battlepoint').classes(
                    #         'text-2xl font-bold text-white'
                    #     )
                    #
                    #     with ui.element('div').classes('bg-white p-2 rounded shadow-md'):
                    #         ui.image('/qr/wifi2.svg').style(
                    #             f'width: {QR_SIZE}px; height: {QR_SIZE}px;'
                    #         )
                    #
                    #     ui.label('Password:').classes('text-2xl font-bold text-white')
                    #     ui.label('BattlePoint123').classes('text-2xl text-white')

            # -----------------------------
            # Settings / Debug buttons
            # -----------------------------
            with ui.row().classes('w-full justify-center gap-4 mt-6'):
                ui.button(
                    '⚙️ Settings',
                    on_click=lambda: ui.navigate.to('/settings'),
                ).props('outline color=white')
                ui.button(
                    '🔧 Debug',
                    on_click=lambda: ui.navigate.to('/debug'),
                ).props('outline color=white')

    # -----------------------------
    # GAME STATUS POLLING
    # -----------------------------
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
            _check(make_absolute_url('/api/koth/state'), koth_status),
            _check(make_absolute_url('/api/3cp/state'), threecp_status),
            _check(make_absolute_url('/api/ad/state'), ad_status),
            _check(make_absolute_url('/api/clock/state'), clock_status),
        )

    ui.timer(0.2, check_running_states)
