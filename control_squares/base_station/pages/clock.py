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
from settings import UnifiedSettingsManager, ThreeCPOptions
import aiohttp
from http_client import get_session, close_session
from pages.overlays import install_winner_overlay




@ui.page('/clock')
async def clock_game_ui():
    """Simple clock-only game interface (no control points)"""
    ui.colors(primary='#1976D2')

    ui.add_head_html(CLOCK_HEAD_HTML)

    with ui.element('div').classes('bp-root-clock'):
        # TOP BAR
        with ui.element('div').classes('bp-topbar-clock'):
            attach_sound_opt_in()
            with ui.row().classes('gap-2 items-center'):
                ui.button('← HOME', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.label('CLOCK MODE').classes('text-white text-lg font-bold')
            ui.label('Set round time, then press START').classes('text-sm text-gray-400')

        # MAIN CONTENT
        with ui.element('div').classes('bp-main-clock'):
            # Big clock
            clock_label = ui.label('10:00').classes('bp-clock-face')

            # Time configuration + start/stop
            with ui.element('div').classes('bp-controls-clock'):
                ui.label('Round Time (mm:ss)').classes('text-2xl font-semibold')

                with ui.element('div').classes('bp-time-input-row'):
                    minutes_input = ui.input(
                        label='Minutes',
                        value='10',
                    ).props(
                        'outlined input-style="font-size: 2.8rem; text-align: center; color: white;"'
                    )
                    seconds_input = ui.input(
                        label='Seconds',
                        value='0',
                    ).props(
                        'outlined input-style="font-size: 2.8rem; text-align: center; color: white;"'
                    )

                with ui.element('div').classes('bp-buttons-row'):
                    start_btn = ui.button('START', icon='play_arrow').props('color=green')
                    stop_btn = ui.button('STOP', icon='stop').props('color=orange')
                    stop_btn.set_visibility(False)

            status_label = ui.label('Waiting...').classes('bp-status-clock')

    # Handlers
    async def start_clock():
        try:
            mins = int(minutes_input.value.strip() or '0')
        except Exception:
            mins = 0
        try:
            secs = int(seconds_input.value.strip() or '0')
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
                    seconds_input.value = f'{s0}'
                    minutes_input.update()
                    seconds_input.update()
                    initialized['done'] = True

                # Button visibility
                if running:
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)

                # Clock display
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
