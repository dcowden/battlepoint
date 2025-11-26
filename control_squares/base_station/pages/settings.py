from nicegui import ui
import aiohttp
from util import make_absolute_url

def parse_squares(text: str) -> list[int]:
    out: list[int] = []
    for token in text.split(','):
        token = token.strip()
        if not token:
            continue
        digits = ''.join(ch for ch in token if ch.isdigit())
        if not digits:
            continue
        try:
            out.append(int(digits))
        except ValueError:
            continue
    return out

@ui.page('/settings')
async def settings_ui(mode: str = 'koth'):
    ui.colors(primary='#1976D2')

    # Allow koth, 3cp, ad
    current_mode = mode if mode in ['koth', '3cp', 'ad'] else 'koth'
    selected_label_map = {'koth': 'KOTH', '3cp': '3CP', 'ad': 'AD'}
    selected_label = selected_label_map[current_mode]
    print("Running Settings Page")
    with ui.column().classes('w-full p-8 max-w-4xl mx-auto'):
        with ui.row().classes('w-full items-center justify-between mb-4'):
            ui.label(f'{current_mode.upper()} Settings').classes('text-3xl font-bold')
            ui.button(
                '← Back',
                on_click=lambda: ui.navigate.to(f'/{current_mode}'),
            ).classes('mb-4')

        # Mode selector
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
            ui.navigate.to(f'/settings?mode={new_mode}')

        mode_tabs.on('update:model-value', on_mode_change)

        with ui.tab_panels(mode_tabs, value=selected_label).classes('w-full'):
            # -------------------- KOTH Settings --------------------
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
                    'Button Threshold (seconds)', value=5, min=1, max=30
                ).classes('w-full')
                koth_start_delay = ui.number(
                    'Start Delay (seconds)', value=5, min=0, max=60
                ).classes('w-full')

                async def save_koth():
                    mode_map = {'KOTH': 0, 'AD': 1, 'CP': 2}
                    payload = {
                        'mode': mode_map[koth_mode_select.value],
                        'time_limit_seconds': int(koth_time_limit.value),
                        'capture_seconds': int(koth_capture_time.value),
                        'capture_button_threshold_seconds': int(koth_button_threshold.value),
                        'start_delay_seconds': int(koth_start_delay.value),
                    }

                    print('Saving KOTH settings with payload:', payload)

                    try:
                        async with aiohttp.ClientSession() as session:
                            async with session.post(
                                make_absolute_url('/api/koth/configure'), json=payload
                            ) as resp:
                                text = await resp.text()
                                print('KOTH /configure status:', resp.status, 'body:', text)
                                if resp.status != 200:
                                    ui.notify(
                                        f'KOTH configure failed: {resp.status}', type='negative'
                                    )
                                    return

                            async with session.post(
                                make_absolute_url('/api/koth/settings/save')
                            ) as resp:
                                text = await resp.text()
                                print('KOTH /settings/save status:', resp.status, 'body:', text)
                                if resp.status != 200:
                                    ui.notify(f'KOTH save failed: {resp.status}', type='negative')
                                    return

                        ui.notify('KOTH settings saved!', type='positive')

                    except Exception as e:
                        print(f'Error saving KOTH settings: {e}')
                        ui.notify(f'Error saving KOTH settings: {e}', type='negative')

                ui.button(
                    'Save KOTH Settings', on_click=save_koth
                ).props('color=primary').classes('mt-4')

            # -------------------- 3CP Settings --------------------
            with ui.tab_panel('3CP'):
                threecp_time_limit = ui.number(
                    'Time Limit (seconds)', value=600, min=20, max=3600
                ).classes('w-full')
                threecp_capture_time = ui.number(
                    'Capture Time (seconds)', value=20, min=2, max=120
                ).classes('w-full')
                threecp_button_threshold = ui.number(
                    'Button Threshold (seconds)', value=5, min=1, max=30
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

                ui.separator().classes('my-4')
                ui.label('Control Square Mappings').classes('text-xl font-bold mb-2')
                ui.label(
                    'Assign control squares (CS-XX) to each control point:'
                ).classes('text-sm text-gray-600 mb-4')

                cp1_input = ui.input(
                    label='CP-Red Squares (comma-separated IDs)',
                    placeholder='1, 2',
                    value='1',
                ).classes('w-full')

                cp2_input = ui.input(
                    label='CP-Center Squares (comma-separated IDs)',
                    placeholder='3, 4',
                    value='3',
                ).classes('w-full')

                cp3_input = ui.input(
                    label='CP-Blue Squares (comma-separated IDs)',
                    placeholder='5',
                    value='5',
                ).classes('w-full')

                async def save_3cp():

                    cp1_squares = parse_squares(cp1_input.value)
                    cp2_squares = parse_squares(cp2_input.value)
                    cp3_squares = parse_squares(cp3_input.value)

                    payload = {
                        'time_limit_seconds': int(threecp_time_limit.value),
                        'capture_seconds': int(threecp_capture_time.value),
                        'capture_button_threshold_seconds': int(threecp_button_threshold.value),
                        'start_delay_seconds': int(threecp_start_delay.value),
                        'add_time_per_first_capture_seconds': int(threecp_time_add.value),
                        'control_square_mapping': {
                            'cp_1_squares': cp1_squares,
                            'cp_2_squares': cp2_squares,
                            'cp_3_squares': cp3_squares,
                        },
                    }

                    print('Saving 3CP settings with payload:', payload)

                    async with aiohttp.ClientSession() as session:
                        async with session.post(
                            make_absolute_url('/api/3cp/settings/save'), json=payload
                        ) as resp:
                            text = await resp.text()
                            print('3CP /settings/save ->', resp.status, text)
                            if resp.status != 200:
                                ui.notify(f'3CP save failed: {resp.status}', type='negative')
                                return

                    ui.notify('3CP settings saved!', type='positive')

                ui.button(
                    'Save 3CP Settings', on_click=save_3cp
                ).props('color=primary').classes('mt-4')

            # -------------------- AD Settings --------------------
            with ui.tab_panel('AD'):
                ad_time_limit = ui.number(
                    'Time Limit (seconds)', value=600, min=5, max=3600
                ).classes('w-full')
                ad_capture_time = ui.number(
                    'Capture Time (seconds)', value=20, min=2, max=120
                ).classes('w-full')
                ad_button_threshold = ui.number(
                    'Button Threshold (seconds)', value=5, min=1, max=30
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

                ui.separator().classes('my-4')
                ui.label('Control Square Mappings').classes('text-xl font-bold mb-2')
                ui.label(
                    'Assign control squares (CS-XX) to each attack/defend point (1,2,3). '
                    'If a point has no squares, it is not used.'
                ).classes('text-sm text-gray-600 mb-4')

                cp1_input_ad = ui.input(
                    label='CP-1 Squares (comma-separated IDs)',
                    placeholder='1, 2',
                    value='1',
                ).classes('w-full')

                cp2_input_ad = ui.input(
                    label='CP-2 Squares (comma-separated IDs)',
                    placeholder='3, 4',
                    value='',
                ).classes('w-full')

                cp3_input_ad = ui.input(
                    label='CP-3 Squares (comma-separated IDs)',
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
                        'capture_button_threshold_seconds': int(ad_button_threshold.value),
                        'start_delay_seconds': int(ad_start_delay.value),
                        'add_time_per_first_capture_seconds': int(ad_time_add.value),
                        'control_square_mapping': {
                            'cp_1_squares': cp1_squares,
                            'cp_2_squares': cp2_squares,
                            'cp_3_squares': cp3_squares,
                        },
                    }

                    print('Saving AD settings with payload:', payload)

                    async with aiohttp.ClientSession() as session:
                        async with session.post(
                            make_absolute_url('/api/ad/settings/save'), json=payload
                        ) as resp:
                            text = await resp.text()
                            print('AD /settings/save ->', resp.status, text)
                            if resp.status != 200:
                                ui.notify(f'AD save failed: {resp.status}', type='negative')
                                return

                    ui.notify('AD settings saved!', type='positive')

                ui.button(
                    'Save AD Settings', on_click=save_ad
                ).props('color=primary').classes('mt-4')

        # Load settings on page load
        async def load_initial_settings():
            print("load initial settings")
            if current_mode == 'koth':
                # ---- KOTH settings load ----
                try:
                    async with aiohttp.ClientSession() as session:
                        async with session.get(
                            make_absolute_url('/api/koth/settings/load')
                        ) as resp:
                            data = await resp.json()

                    print("[DEBUG] load_initial_settings(koth): got data =", data)

                    if data.get('status') != 'not_found':
                        # mode int -> label
                        mode_rev = {0: 'KOTH', 1: 'AD', 2: 'CP'}
                        koth_mode_select.value = mode_rev.get(data.get('mode', 0), 'KOTH')

                        koth_time_limit.value = data.get('time_limit_seconds', 60)
                        koth_capture_time.value = data.get('capture_seconds', 20)
                        koth_button_threshold.value = data.get(
                            'capture_button_threshold_seconds', 5
                        )
                        koth_start_delay.value = data.get('start_delay_seconds', 5)

                        print(
                            "[DEBUG] load_initial_settings(koth): "
                            "mode=", koth_mode_select.value,
                            "time_limit=", koth_time_limit.value,
                            "capture=", koth_capture_time.value,
                            "btn_thresh=", koth_button_threshold.value,
                            "start_delay=", koth_start_delay.value,
                        )

                        koth_mode_select.update()
                        koth_time_limit.update()
                        koth_capture_time.update()
                        koth_button_threshold.update()
                        koth_start_delay.update()

                except Exception as e:
                    print(f"[DEBUG] Error loading KOTH settings: {e}")

            elif current_mode == '3cp':
                # ---- 3CP settings load ----
                try:
                    async with aiohttp.ClientSession() as session:
                        async with session.get(
                            make_absolute_url('/api/3cp/settings/load')
                        ) as resp:
                            data = await resp.json()

                    print("[DEBUG] load_initial_settings(3cp): got data =", data)

                    if data.get('status') != 'not_found':
                        threecp_time_limit.value = data.get('time_limit_seconds', 600)
                        threecp_capture_time.value = data.get('capture_seconds', 20)
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

                        print(
                            "[DEBUG] load_initial_settings(3cp): "
                            "assigned values ->",
                            "time_limit=", threecp_time_limit.value,
                            "capture=", threecp_capture_time.value,
                            "btn_thresh=", threecp_button_threshold.value,
                            "start_delay=", threecp_start_delay.value,
                            "add_time=", threecp_time_add.value,
                            "cp1=", cp1_input.value,
                            "cp2=", cp2_input.value,
                            "cp3=", cp3_input.value,
                        )

                        threecp_time_limit.update()
                        threecp_capture_time.update()
                        threecp_button_threshold.update()
                        threecp_start_delay.update()
                        threecp_time_add.update()
                        cp1_input.update()
                        cp2_input.update()
                        cp3_input.update()
                except Exception as e:
                    print(f"[DEBUG] Error loading 3CP settings: {e}")

            else:
                # ---- AD settings load ----
                try:
                    async with aiohttp.ClientSession() as session:
                        async with session.get(
                            make_absolute_url('/api/ad/settings/load')
                        ) as resp:
                            data = await resp.json()

                    print("[DEBUG] load_initial_settings(ad): got data =", data)

                    if data.get('status') != 'not_found':
                        ad_time_limit.value = data.get('time_limit_seconds', 600)
                        ad_capture_time.value = data.get('capture_seconds', 20)
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

                        print(
                            "[DEBUG] load_initial_settings(ad): "
                            "assigned values ->",
                            "time_limit=", ad_time_limit.value,
                            "capture=", ad_capture_time.value,
                            "btn_thresh=", ad_button_threshold.value,
                            "start_delay=", ad_start_delay.value,
                            "add_time=", ad_time_add.value,
                            "cp1=", cp1_input_ad.value,
                            "cp2=", cp2_input_ad.value,
                            "cp3=", cp3_input_ad.value,
                        )

                        ad_time_limit.update()
                        ad_capture_time.update()
                        ad_button_threshold.update()
                        ad_start_delay.update()
                        ad_time_add.update()
                        cp1_input_ad.update()
                        cp2_input_ad.update()
                        cp3_input_ad.update()

                except Exception as e:
                    print(f"[DEBUG] Error loading AD settings: {e}")

        await load_initial_settings()
