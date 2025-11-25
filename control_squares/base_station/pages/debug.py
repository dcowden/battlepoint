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


@ui.page('/debug')
def debug_ui(from_page: str = '', admin: str = ''):
    ui.colors(primary='#1976D2')
    came_from_mobile = (from_page == 'mobile')

    with ui.column().classes('w-full p-8 max-w-6xl mx-auto gap-4'):
        ui.label('Debug Console').classes('text-3xl font-bold')

        def go_back():
            if came_from_mobile:
                target = f'/mobile?admin={admin}' if admin else '/mobile'
                ui.navigate.to(target)
            else:
                ui.navigate.to('/')

        ui.button('← BACK TO GAME', on_click=go_back)

        # Manual control switch
        with ui.row().classes('items-center gap-4'):
            manual_switch = ui.switch(
                'Manual Proximity Control (use Red/Blue toggles on main screen)'
            )
            manual_status = ui.label('').classes('text-sm text-gray-400')

        # BLE SCANNER CARD
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full justify-between items-center mb-4'):
                ui.label('🔵 Bluetooth LE Scanner').classes('text-2xl font-bold')
                with ui.row().classes('gap-2'):
                    start_scan_btn = ui.button('▶ START SCAN').props('color=green')
                    stop_scan_btn = ui.button('⏹ STOP').props('color=red')
                    stop_scan_btn.set_visibility(False)

            ble_status = ui.label('Status: Not scanning').classes('text-lg mb-2')
            device_count_label = ui.label('Tracking 0 device(s)').classes(
                'text-md text-gray-400 mb-4'
            )

            ble_columns = [
                {
                    'name': 'name',
                    'label': 'Device Name',
                    'field': 'name',
                    'align': 'left',
                    'sortable': True,
                },
                {
                    'name': 'rssi',
                    'label': 'RSSI (dBm)',
                    'field': 'rssi',
                    'align': 'center',
                    'sortable': True,
                },
                {
                    'name': 'address',
                    'label': 'Address',
                    'field': 'address',
                    'align': 'left',
                },
                {
                    'name': 'last_seen',
                    'label': 'Last Seen (ms ago)',
                    'field': 'last_seen_ms',
                    'align': 'center',
                    'sortable': True,
                },
                {
                    'name': 'mfg_data',
                    'label': 'Manufacturer Data',
                    'field': 'manufacturer_data',
                    'align': 'left',
                },
            ]
            ble_table = ui.table(columns=ble_columns, rows=[], row_key='address').classes(
                'w-full'
            )
            ble_table.add_slot(
                'body-cell-rssi',
                """
                <q-td :props="props">
                    <q-badge :color="props.value > -60 ? 'green' : props.value > -80 ? 'orange' : 'red'">
                        {{ props.value }}
                    </q-badge>
                </q-td>
            """,
            )

        # EVENT LOG CARD
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full items-center justify-between mb-2'):
                ui.label('📋 Event Log').classes('text-xl font-bold')
                event_count_label = ui.label('0 events').classes('text-sm text-gray-400')

            event_box = ui.column().classes(
                'w-full bg-gray-50 rounded p-2 gap-1 dark:bg-gray-900'
            ).style('height: calc(50vh - 360px); overflow-y: auto;')
            event_box_id = f"bp-event-box-{event_box.id}"
            event_box.props(f'id={event_box_id}')

        # GAME STATE CARD
        with ui.card().classes('w-full p-4'):
            ui.label('⚙️ Game State').classes('text-xl font-bold mb-2')
            state_display = ui.json_editor({}).classes('w-full')

    # remember scroll
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

    session_holder: dict[str, aiohttp.ClientSession | None] = {'session': None}
    last_events: list[str] = []


    async def _cleanup(_msg):
        if session_holder['session'] is not None:
            await session_holder['session'].close()
            session_holder['session'] = None

    event_box.on('disconnect', _cleanup)

    async def sync_manual_from_backend():
        try:
            s = await get_session()
            async with s.get('http://localhost:8080/api/manual/state') as resp:
                st = await resp.json()
            enabled = bool(st.get('manual_control'))
            manual_switch.value = enabled
            manual_switch.update()
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'} "
                "(BLE scanning still runs; in manual mode BLE does not change proximity)."
            )
        except Exception as e:
            print(f"[debug] manual sync failed: {e}")

    async def on_manual_change(_e):
        try:
            s = await get_session()
            enabled = bool(manual_switch.value)
            await s.post(
                f'http://localhost:8080/api/manual/mode/{str(enabled).lower()}'
            )
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'} "
                "(BLE scanning still runs; in manual mode BLE does not change proximity)."
            )
            ui.notify(
                f"Manual control {'enabled' if enabled else 'disabled'}", type='info'
            )
        except Exception as ex:
            print(f"[debug] manual toggle failed: {ex}")

    manual_switch.on('update:model-value', on_manual_change)

    async def start_ble_scan():
        s = await get_session()
        await s.post('http://localhost:8080/api/bluetooth/start')
        start_scan_btn.set_visibility(False)
        stop_scan_btn.set_visibility(True)
        ble_status.set_text('Status: Scanning...')
        ble_status.classes('text-green-400', remove='text-gray-400')
        ui.notify('BLE scanning started', type='positive')

    async def stop_ble_scan():
        s = await get_session()
        await s.post('http://localhost:8080/api/bluetooth/stop')
        start_scan_btn.set_visibility(True)
        stop_scan_btn.set_visibility(False)
        ble_status.set_text('Status: Not scanning')
        ble_status.classes('text-gray-400', remove='text-green-400')
        ui.notify('BLE scanning stopped', type='info')

    start_scan_btn.on('click', start_ble_scan)
    stop_scan_btn.on('click', stop_ble_scan)

    def _set_scan_buttons_enabled(enabled: bool):
        if enabled:
            start_scan_btn.enable()
            stop_scan_btn.enable()
        else:
            start_scan_btn.disable()
            stop_scan_btn.disable()

    async def update_debug_once():
        if state_display.client is None:
            return

        s = await get_session()

        try:
            async with s.get('http://localhost:8080/api/koth/state') as resp:
                state = await resp.json()
                state_display.value = state
                state_display.update()

                game_running = bool(state.get('running', False))
                _set_scan_buttons_enabled(not game_running)

                events = state.get('events', [])
                event_count_label.set_text(f'{len(events)} event(s)')

                if events != last_events:
                    event_box.clear()
                    with event_box:
                        for line in reversed(events):
                            ui.label(line).classes(
                                'text-sm text-gray-900 dark:text-gray-100'
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

            async with s.get('http://localhost:8080/api/bluetooth/devices') as resp:
                ble_data = await resp.json()

                if ble_data.get('scanning'):
                    ble_status.set_text('Status: Scanning...')
                    ble_status.classes('text-green-400', remove='text-gray-400')
                    start_scan_btn.set_visibility(False)
                    stop_scan_btn.set_visibility(True)
                else:
                    ble_status.set_text('Status: Not scanning')
                    ble_status.classes('text-gray-400', remove='text-green-400')
                    start_scan_btn.set_visibility(True)
                    stop_scan_btn.set_visibility(False)

                device_count = ble_data.get('device_count', 0)
                device_count_label.set_text(f'Tracking {device_count} device(s)')

                devices = ble_data.get('devices', [])
                ble_table.rows = devices
                ble_table.update()

        except Exception as e:
            print(f"Debug update error: {e}")

    ui.timer(0.25, update_debug_once, once=False)

