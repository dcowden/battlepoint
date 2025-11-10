"""
BattlePoint - Real-life Team Fortress 2 Game
Python/NiceGUI Implementation with BLE Scanner + Manual Control Toggle
"""

import asyncio
import json
import os
import random
import sys
import threading
import traceback
from enum import Enum
from typing import Optional, List, Dict

import aiohttp
import pygame
from bleak import BleakScanner
from nicegui import ui, app,Client
from starlette.staticfiles import StaticFiles
from battlepoint_core import (
    Clock,
    Team,
    BluetoothTag,
    TagType,
    EventManager,
    TeamColor,
    Proximity,
    GameOptions,
    LedMeter,
    GameMode,
    team_text,
    RealClock,
    ControlPoint
)
from battlepoint_game import create_game, BaseGame, EnhancedGameBackend,SoundSystem,SOUND_MAP
from ble_scanner import EnhancedBLEScanner

# Windows-specific fix
if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

GAME_LOOP_SECS = 100 / 1000  # 10 Hz

# Single backend instance

app.mount('/sounds',StaticFiles(directory='sounds'), name='sounds')

class BrowserSoundClient:
    def __init__(self, client_id: int, audio: ui.audio):
        self.client_id = client_id
        self.audio = audio
        self.enabled = False



from typing import Dict, Optional
import random

from nicegui import ui, app, background_tasks, Client  # make sure background_tasks, Client are imported

class BrowserSoundBus:
    def __init__(self, base_url: str = '/sounds'):
        self.base_url = base_url.rstrip('/')
        self.clients: dict[str, Client] = {}
        self.enabled: set[str] = set()
        self.volume: int = 10
        self.sound_map = SOUND_MAP
        self._menu_tracks = list(range(18, 26))

    def _src_for_id(self, sound_id: int) -> str | None:
        fn = self.sound_map.get(sound_id)
        if not fn:
            print(f"[BROWSER_SOUND] unknown sound id {sound_id}")
            return None
        return f'{self.base_url}/{fn}'

    def register_client(self, client: Client) -> None:
        self.clients[client.id] = client
        print(f"[BROWSER_SOUND] register client {client.id}")
        with client:
            if app.storage.user.get('bp_sound_enabled'):
                self.enable_for_client(client.id, from_auto=True)

    def unregister_client(self, client: Client) -> None:
        cid = client.id
        self.clients.pop(cid, None)
        self.enabled.discard(cid)
        print(f"[BROWSER_SOUND] unregister client {cid}")

    def enable_for_client(self, client_id: str, from_auto: bool = False) -> None:
        if client_id in self.clients:
            self.enabled.add(client_id)
            print(f"[BROWSER_SOUND] {'auto-' if from_auto else ''}enabled client {client_id}")

    def disable_for_client(self, client_id: str) -> None:
        self.enabled.discard(client_id)
        print(f"[BROWSER_SOUND] disabled client {client_id}")

    def set_volume(self, volume: int) -> None:
        self.volume = max(0, min(30, int(volume)))
        print(f"[BROWSER_SOUND] set_volume({self.volume})")

    def _broadcast_js(self, js: str) -> None:
        vol = self.volume / 30.0  # if needed in js
        for cid in list(self.enabled):
            client = self.clients.get(cid)
            if not client:
                self.enabled.discard(cid)
                continue
            try:
                with client:
                    client.run_javascript(js)
            except Exception as e:
                print(f"[BROWSER_SOUND] error on client {cid}: {e}")
                self.enabled.discard(cid)

    # ---- API used by backend ----

    def stop(self) -> None:
        # stop music channel on all
        self._broadcast_js('window.bpAudio && bpAudio.stopChannel("music");')
        print("[BROWSER_SOUND] stop() music")

    def play_menu_track(self) -> None:
        if not self._menu_tracks:
            return
        src = self._src_for_id(random.choice(self._menu_tracks))
        if not src:
            return
        vol = self.volume / 30.0
        self._broadcast_js(
            f'window.bpAudio && bpAudio.playChannel("music", {{src: "{src}", volume: {vol:.3f}, loop: true}});'
        )
        print(f"[BROWSER_SOUND] play_menu_track {src}")

    def loop(self, sound_id: int) -> None:
        # if you ever need a looping announcer/etc, map to a channel
        self.play(sound_id)

    def queue(self, sound_id: int) -> None:
        # for now, treat queue as stomp; if you truly need queueing,
        # we can extend bpAudio with a FIFO.
        self.play(sound_id)

    def play(self, sound_id: int) -> None:
        src = self._src_for_id(sound_id)
        if not src:
            return
        vol = self.volume / 30.0

        # decide channel behavior:
        # - announcers: single 'announcer' channel, stomp previous
        # - everything else: sfx (can overlap) or tune as needed
        if sound_id in {10,11,12,19,21,25,31,29,27,40,37,35,43,42,41,39,36,3,2,14}:  # your announcer IDs etc.
            js = f'window.bpAudio && bpAudio.playChannel("announcer", {{src: "{src}", volume: {vol:.3f}}});'
        else:
            js = f'window.bpAudio && bpAudio.sfx({{src: "{src}", volume: {vol:.3f}}});'

        self._broadcast_js(js)
        print(f"[BROWSER_SOUND] play {sound_id} -> {src}")
browser_sound_bus = BrowserSoundBus( base_url='/sounds')

def _on_connect(client: Client):
    browser_sound_bus.register_client(client)

def _on_disconnect(client: Client):
    browser_sound_bus.unregister_client(client)

app.on_connect(_on_connect)
app.on_disconnect(_on_disconnect)


def inject_howler_and_manager():
    ui.add_head_html('''
    <script src="https://cdn.jsdelivr.net/npm/howler@2.2.4/dist/howler.min.js"></script>
    <script>
    window.bpAudio = window.bpAudio || {
      primed: false,

      prime: async function() {
        if (this.primed) return true;
        try {
          const s = new Howl({
            src: ["data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEAIlYAAESsAAACABAAZGF0YQAAAAA="],
            volume: 0.0,
          });
          await s.play();
          this.primed = true;
          return true;
        } catch(e) {
          console.warn("bpAudio prime failed", e);
          return false;
        }
      },

      channels: {},

      playChannel: function(name, opts) {
        try {
          // stop previous on this channel
          const existing = this.channels[name];
          if (existing) {
            try { existing.stop(); existing.unload(); } catch(e) {}
          }
          const howl = new Howl({
            src: [opts.src],
            volume: opts.volume ?? 1.0,
            loop: !!opts.loop,
          });
          howl.play();
          this.channels[name] = howl;
        } catch(e) {
          console.error("bpAudio.playChannel error", name, e);
        }
      },

      stopChannel: function(name) {
        const h = this.channels[name];
        if (!h) return;
        try { h.stop(); h.unload(); } catch(e) {}
        delete this.channels[name];
      },

      // fire-and-forget sfx (can overlap)
      sfx: function(opts) {
        try {
          const h = new Howl({
            src: [opts.src],
            volume: opts.volume ?? 1.0,
            loop: false,
          });
          h.play();
        } catch(e) {
          console.error("bpAudio.sfx error", e);
        }
      },
    };
    </script>
    ''')

def attach_sound_opt_in():
    inject_howler_and_manager()

    client = ui.context.client

    with client:
        if app.storage.user.get('bp_sound_enabled'):
            browser_sound_bus.enable_for_client(client.id, from_auto=True)
            return

    btn = ui.button('Enable sound').props('color=yellow-7 unelevated').classes('q-ml-md')

    async def _enable_for_this_client():
        ok = await ui.run_javascript('window.bpAudio ? window.bpAudio.prime() : true;', timeout=2.0)
        if not ok:
            ui.notify('Browser blocked audio. Tap again or check autoplay settings.', type='warning')
            return

        with ui.context.client:
            app.storage.user['bp_sound_enabled'] = True
        browser_sound_bus.enable_for_client(ui.context.client.id)
        btn.set_visibility(False)
        ui.notify('Game sounds enabled on this device.', type='positive')

    btn.on('click', _enable_for_this_client)

# ========================================================================
# FASTAPI / NICEGUI ENDPOINTS
# ========================================================================



_game_loop_task = None
enhanced_backend = EnhancedGameBackend(sound_system = browser_sound_bus)

async def game_loop():
    while True:
        enhanced_backend.update()
        await asyncio.sleep(GAME_LOOP_SECS)


@app.get("/api/state")
async def get_state():
    return enhanced_backend.get_state()


@app.post("/api/start")
async def start_game_endpoint():
    enhanced_backend.start_game()
    return {"status": "countdown"}


@app.post("/api/stop")
async def stop_game_endpoint():
    enhanced_backend.stop_game()
    return {"status": "stopped"}


@app.post("/api/configure")
async def configure(options: dict):
    go = GameOptions(
        mode=GameMode(options.get('mode', 0)),
        capture_seconds=options.get('capture_seconds', 20),
        capture_button_threshold_seconds=options.get('capture_button_threshold_seconds', 5),
        time_limit_seconds=options.get('time_limit_seconds', 60),
        start_delay_seconds=options.get('start_delay_seconds', 5),
    )
    enhanced_backend.configure(go)
    return {"status": "configured"}


@app.post("/api/proximity/red")
async def red_proximity():
    enhanced_backend.proximity.red_button_press()
    return {"status": "ok"}


@app.post("/api/proximity/blue")
async def blue_proximity():
    enhanced_backend.proximity.blu_button_press()
    return {"status": "ok"}


# Manual control endpoints

@app.post("/api/manual/mode/{enabled}")
async def set_manual_mode(enabled: bool):
    enhanced_backend.set_manual_control(enabled)
    return enhanced_backend.get_manual_state()


@app.post("/api/manual/red/{on}")
async def set_manual_red(on: bool):
    enhanced_backend.set_manual_state(red=on)
    return enhanced_backend.get_manual_state()


@app.post("/api/manual/blu/{on}")
async def set_manual_blu(on: bool):
    enhanced_backend.set_manual_state(blu=on)
    return enhanced_backend.get_manual_state()


@app.get("/api/manual/state")
async def get_manual_state():
    return enhanced_backend.get_manual_state()


@app.post("/api/settings/save")
async def save_settings():
    success = enhanced_backend.save_settings()
    return {"status": "saved" if success else "error"}


@app.get("/api/settings/load")
async def load_settings():
    saved = enhanced_backend.settings_manager.load_settings()
    if saved:
        options, volume, _ = saved
        return {
            "mode": options.mode.value,
            "capture_seconds": options.capture_seconds,
            "capture_button_threshold_seconds": options.capture_button_threshold_seconds,
            "time_limit_seconds": options.time_limit_seconds,
            "start_delay_seconds": options.start_delay_seconds,
            "volume": volume,
        }
    return {"status": "not_found"}


@app.post("/api/sound/play/{sound_id}")
async def play_sound(sound_id: int):
    enhanced_backend.sound_system.play(sound_id)
    return {"status": "playing"}


@app.post("/api/sound/volume/{volume}")
async def set_volume(volume: int):
    enhanced_backend.sound_system.set_volume(volume)
    return {"volume": enhanced_backend.sound_system.volume}


# BLE scanner endpoints

@app.get("/api/bluetooth/devices")
async def get_bluetooth_devices():
    return enhanced_backend.get_ble_devices_summary()


@app.get("/api/bluetooth/tags")
async def get_bluetooth_tags():
    tags = enhanced_backend.scanner.get_active_tags()
    return {
        "tags": [
            {
                "id": tag.id,
                "type": tag.tag_type.name,
                "team": team_text(tag.team),
                "last_seen": tag.last_seen,
            }
            for tag in tags
        ]
    }


@app.post("/api/bluetooth/start")
async def start_bluetooth():
    await enhanced_backend.start_bluetooth_scanning()
    return {"status": "scanning"}


@app.post("/api/bluetooth/stop")
async def stop_bluetooth():
    await enhanced_backend.stop_bluetooth_scanning()
    return {"status": "stopped"}


# ========================================================================
# MAIN UI (/)
# ========================================================================

import asyncio as _asyncio  # alias


@ui.page('/')
async def game_ui():
    ui.colors(primary='#1976D2')


    ui.add_head_html("""
    <style>
      html, body {
        margin: 0;
        padding: 0;
        background: #000;
        height: 100%;
        overflow: hidden;
      }
      .bp-root {
        width: 100vw;
        height: 100vh;
        background: #000;
        display: flex;
        flex-direction: column;
      }
      .bp-topbar {
        height: 52px;
        background: #111;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 0 1rem;
        gap: 4 rem;
      }
      .bp-topbar-left,
      .bp-topbar-center,
      .bp-topbar-right {
        display: flex;
        align-items: center;
        gap: 0.5rem;
      }
      .bp-topbar-center { justify-content: center; }
      .bp-main {
        height: calc(100vh - 52px - 170px);
        display: flex;
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
        overflow: hidden;
      }
      .bp-side {
        width: 20vw;
        min-width: 200px;
        height: 100%;
        display: flex;
        flex-direction: column;
        gap: 0.75rem;
        align-items: center;
        justify-content: center;
      }
      .bp-vert-shell {
        width: 270px;
        height: 100vh;
        max-height: calc(100vh - 52px - 160px);
        background: #222;
        border: 3px solid #444;
        border-radius: 16px;
        position: relative;
        overflow: hidden;
      }
      
        .bp-status-row {
          display: flex;
          flex-wrap: wrap;          /* lets them wrap on narrow screens */
          gap: 1.5rem;              /* space between items */
          align-items: baseline;
          justify-content: center;  /* center under the capture bar */
          color: #ddd;
          font-size: 1.2rem;
        }
            
      .bp-vert-fill {
        position: absolute;
        bottom: 0;
        left: 0;
        width: 100%;
        height: 0%;
        background: #ff0000;
        transition: height 0.15s linear;
      }
      .bp-center {
        flex: 1;
        height: 100%;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 1.0rem;
      }
      .bp-clock {
        flex: 1 1 auto;
        font-size: clamp(10rem, 40rem, 80rem);
        line-height: 1;
        font-weight: 400;
        color: #fff;
        text-align: center;
        max-height: 64%;
      }
      .bp-status {
        font-size: 1.4rem;
        color: #ddd;
      }
     .bp-capture-mult {
       position: absolute;
       top: 50%;
       left: 50%;
       transform: translate(-50%, -50%);
       font-size: 10rem;
       font-weight: 800;
       color: #ffffff;
       text-shadow: 0 0 12px rgba(0, 0, 0, 0.9);
       pointer-events: none;
     }
      
      .bp-capture-shell {
        width: min(60vw, 900px);
        flex-shrink: 0; 
        height: 150px;
        background: #222;
        border: 2px solid #444;
        border-radius: 12px;
        overflow: hidden;
        position: relative;
        margin: 2.5rem;
      }
      .bp-capture-fill {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 0%;
        background: #6600ff;
        transition: width 0.12s linear;
      }
      .bp-bottom {
        height: 150px;
        display: flex;
        flex-direction: column;
        gap: 0.4rem;
        padding: 1.4rem;
      }
      .bp-bottom-wrapper {
        position: relative;
        width: 100%;
        height: 100%;
      }
      .bp-horiz-shell {
        width: 100%;
        height: 100%;
        background: #000000;
        position: relative;
        overflow: hidden;
      }
      .bp-horiz-red {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 50%;
        background: #000000;
        transition: width 0.15s linear;
      }
    </style>
    
    """)

    with ui.element('div').classes('bp-root'):

        # TOP BAR
        with ui.element('div').classes('bp-topbar'):
            with ui.element('div').classes('bp-topbar-left'):
                ui.label('BattlePoint').classes('text-white text-lg font-bold')

            with ui.element('div').classes('bp-topbar-center'):

                # BLU toggle: OFF = grey, ON = blue
                blue_toggle = ui.toggle(
                    options=['B OFF', 'B ON'],
                    value='B OFF',
                ).props(
                    'dense push toggle-color=blue color=grey-8 text-color=white'
                )

                # RED toggle: OFF = grey, ON = red
                red_toggle = ui.toggle(
                    options=['R OFF', 'R ON'],
                    value='R OFF',
                ).props(
                    'dense push toggle-color=red color=grey-8 text-color=white'
                )



                start_btn = ui.button('START').props('color=green')
                stop_btn = ui.button('STOP').props('color=orange')
                stop_btn.set_visibility(False)

            with ui.element('div').classes('bp-topbar-right'):
                attach_sound_opt_in()
                ui.button('Game', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.button('Settings', on_click=lambda: ui.navigate.to('/settings')).props('flat color=white')
                ui.button('Debug', on_click=lambda: ui.navigate.to('/debug')).props('flat color=white')

        # MAIN
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

                # one horizontal row, still separate variables
                with ui.element('div').classes('bp-status-row'):
                    status_label = ui.label('Waiting...').classes('bp-status-label')
                    cp_owner = ui.label('Owner: ---').classes('bp-status-label')
                    cp_capturing = ui.label('Capturing: ---').classes('bp-status-label')
                    cp_contested = ui.label('Contested: False').classes('bp-status-label')

            with ui.element('div').classes('bp-side'):
                ui.label('RED').classes('text-8xl font-bold text-red-400')
                with ui.element('div').classes('bp-vert-shell'):
                    blue_fill = ui.element('div').classes('bp-vert-fill')

        # bottom owner meter
        with ui.element('div').classes('bp-bottom'):
            with ui.element('div').classes('bp-bottom-wrapper'):
                with ui.element('div').classes('bp-horiz-shell'):
                    horiz_red = ui.element('div').classes('bp-horiz-red')

    # Shared HTTP session for this page
    page_session = {'session': None}

    async def get_page_session() -> aiohttp.ClientSession:
        s = page_session.get('session')
        if s is None or s.closed:
            s = aiohttp.ClientSession()
            page_session['session'] = s
        return s

    async def close_page_session():
        s = page_session.get('session')
        if s and not s.closed:
            await s.close()
        page_session['session'] = None

    # ====================================================================
    # WINNER OVERLAY
    # ====================================================================

    winner_dismissed = {'flag': False}
    last_winner_key = {'key': None}

    def _team_bg(team: str) -> str:
        t = (team or '').strip().upper()
        if t == 'RED':
            return '#FF0000'
        if t == 'BLU':
            return '#0000FF'
        return 'rgba(0,0,0,0.95)'

    def _winner_text(team: str) -> str:
        t = (team or '').strip().upper()
        if t in ('RED', 'BLU'):
            return f'Winner: {t}'
        return 'Winner'

    with ui.dialog() as winner_dialog:
        with ui.element('div').style(
            'position:fixed;'
            'left:0;'
            'top:52px;'
            'width:100vw;'
            'height:calc(100vh - 52px);'
            'max-width:none;'
            'max-height:none;'
            'border-radius:0;'
            'box-shadow:none;'
            'display:flex;'
            'flex-direction:column;'
            'align-items:center;'
            'justify-content:center;'
            'background:rgba(0,0,0,0.95);'
            'color:white;'
            'z-index:1;'
        ) as winner_overlay:

            winner_label = ui.label('Winner').classes(
                'text-8xl font-extrabold tracking-wide mb-4'
            )
            winner_subtitle = ui.label('Control point secured').classes('text-3xl')

            ui.button(
                'DISMISS',
                on_click=lambda: (
                    winner_dialog.close(),
                    winner_dismissed.__setitem__('flag', True)
                ),
            ).props('unelevated color=white text-color=black').classes(
                'text-xl px-10 py-4 rounded-xl font-bold'
            ).style(
                'position:absolute;'
                'bottom:40px;'
                'left:50%;'
                'transform:translateX(-50%);'
            )

    def _open_winner(team_str: str):
        bg = _team_bg(team_str)
        winner_overlay.style(
            'position:fixed;'
            'left:0;'
            'top:52px;'
            'width:100vw;'
            'height:calc(100vh - 52px);'
            'max-width:none;'
            'max-height:none;'
            'border-radius:0;'
            'box-shadow:none;'
            'display:flex;'
            'flex-direction:column;'
            'align-items:center;'
            'justify-content:center;'
            f'background:{bg};'
            'color:white;'
            'z-index:1;'
        )
        winner_label.set_text(_winner_text(team_str))
        winner_subtitle.set_text('Control point secured')
        winner_dialog.open()
        winner_dismissed['flag'] = False

    # ====================================================================
    # START / STOP HANDLERS
    # ====================================================================

    async def start_game_click(_e=None):
        try:
            winner_dialog.close()
        except Exception:
            traceback.print_exc()
        winner_dismissed['flag'] = True
        s = await get_page_session()
        await s.post('http://localhost:8080/api/start')

    async def stop_game_click(_e=None):
        s = await get_page_session()
        await s.post('http://localhost:8080/api/stop')

    start_btn.on('click', start_game_click)
    stop_btn.on('click', stop_game_click)

    # ====================================================================
    # MANUAL TEAM TOGGLES
    # ====================================================================

    syncing = {'flag': False}  # avoid recursion when syncing from backend

    async def sync_manual_toggles_from_backend():
        """Refresh toggle positions from backend without retriggering handlers."""
        try:
            syncing['flag'] = True
            s = await get_page_session()
            async with s.get('http://localhost:8080/api/manual/state') as resp:
                st = await resp.json()

            red_val = bool(st.get('red'))
            blu_val = bool(st.get('blu'))

            desired_red = 'R ON' if red_val else 'R OFF'
            desired_blu = 'B ON' if blu_val else 'B OFF'

            if red_toggle.value != desired_red:
                red_toggle.value = desired_red
                red_toggle.update()
            if blue_toggle.value != desired_blu:
                blue_toggle.value = desired_blu
                blue_toggle.update()
        except Exception as e:
            print(f"[UI] sync_manual_toggles error: {e}")
        finally:
            syncing['flag'] = False

    async def _handle_team_toggle(team: str, toggle, label_on: str, label_off: str):
        """Shared handler for RED/BLU manual toggles."""
        if syncing['flag']:
            return
        try:
            s = await get_page_session()

            # Ensure manual mode is enabled
            async with s.get('http://localhost:8080/api/manual/state') as resp:
                st = await resp.json()

            if not st.get('manual_control'):
                ui.notify(
                    'Enable manual control on Debug page to use Red/Blue toggles.',
                    type='warning',
                )
                await sync_manual_toggles_from_backend()
                return

            # NiceGUI has already updated toggle.value here
            new_label = toggle.value
            new_val = (new_label == label_on)

            endpoint = 'red' if team == 'red' else 'blu'
            await s.post(
                f'http://localhost:8080/api/manual/{endpoint}/{str(new_val).lower()}'
            )

        except Exception as ex:
            print(f"[UI] on_{team}_toggle error: {ex}")
            ui.notify(
                f'Error updating {team.upper()} manual state.',
                type='negative',
            )
            await sync_manual_toggles_from_backend()

    async def on_red_toggle(e):
        await _handle_team_toggle('red', red_toggle, 'R ON', 'R OFF')

    async def on_blue_toggle(e):
        await _handle_team_toggle('blu', blue_toggle, 'B ON', 'B OFF')

    # Correct event registration for NiceGUI: listen to update:model-value
    red_toggle.on('update:model-value', on_red_toggle)
    blue_toggle.on('update:model-value', on_blue_toggle)

    # Initial sync from backend
    await sync_manual_toggles_from_backend()

    # ====================================================================
    # UI POLLING
    # ====================================================================

    def _alive(el) -> bool:
        try:
            _ = el.client
            return True
        except RuntimeError:
            return False

    async def update_ui():
        last_events: list[str] = []
        try:
            while _alive(game_clock):
                try:
                    s = await get_page_session()
                    async with s.get('http://localhost:8080/api/state') as resp:
                        state = await resp.json()
                except Exception:
                    await _asyncio.sleep(0.3)
                    continue

                phase = state.get('phase', 'idle')
                winner_team = state.get('winner')

                if phase == 'ended' and winner_team:
                    key = f"{phase}:{winner_team}"
                    if not winner_dismissed['flag'] and last_winner_key['key'] != key:
                        _open_winner(winner_team)
                        last_winner_key['key'] = key
                else:
                    if phase in ('running', 'countdown', 'idle'):
                        try:
                            winner_dialog.close()
                        except Exception:
                            traceback.print_exc()
                        winner_dismissed['flag'] = False
                        last_winner_key['key'] = None

                # Toggle START / STOP visibility based on backend state
                running = bool(state.get('running', False))
                if phase in ('running', 'countdown') or running:
                    # Game in progress: show STOP, hide START
                    start_btn.set_visibility(False)
                    stop_btn.set_visibility(True)
                else:
                    # Idle or ended: show START, hide STOP
                    start_btn.set_visibility(True)
                    stop_btn.set_visibility(False)


                meters = state.get('meters', {})

                m1 = meters.get('timer1')
                if m1 and _alive(red_fill):
                    red_fill.style(
                        f'height: {m1.get("percent", 0)}%; '
                        f'background: {m1.get("fg", "#0000FF")};'
                    )

                m2 = meters.get('timer2')
                if m2 and _alive(blue_fill):
                    blue_fill.style(
                        f'height: {m2.get("percent", 0)}%; '
                        f'background: {m2.get("fg", "#FF0000")};'
                    )

                m_cap = meters.get('capture')
                if m_cap and _alive(capture_fill):
                    pct = m_cap.get('percent', 0)
                    fg = m_cap.get('fg', '#6600ff')
                    capture_fill.style(f'width: {pct}%; background: {fg};')

                m_owner = meters.get('owner')
                if m_owner and _alive(horiz_red):
                    pct = m_owner.get("percent", 50)
                    fg = (m_owner.get("fg") or "#FF0000")
                    if fg.lower() in ("#000000", "black"):
                        pct = 50
                        fg = "#000000"
                    horiz_red.style(f'width: {pct}%; background: {fg};')

                if phase == 'countdown':
                    cd = state.get('countdown_remaining', 0)
                    if _alive(game_clock):
                        game_clock.set_text(f'{cd}')
                    if _alive(status_label):
                        status_label.set_text('COUNTDOWN...')
                else:
                    rem = state.get('remaining_seconds', 0)
                    if _alive(game_clock):
                        game_clock.set_text(f'{rem // 60}:{rem % 60:02d}')
                    if _alive(status_label):
                        if state.get('running'):
                            status_label.set_text('GAME RUNNING')
                        elif state.get('winner'):
                            status_label.set_text(f"Winner: {state['winner']}")
                        else:
                            status_label.set_text('Waiting...')

                cp = state.get('control_point', {})
                if _alive(cp_owner):
                    cp_owner.set_text(f"Owner: {cp.get('owner', '---')}")
                if _alive(cp_capturing):
                    cp_capturing.set_text(f"Capturing: {cp.get('capturing', '---')}")
                if _alive(cp_contested):
                    cp_contested.set_text(f"Contested: {cp.get('contested', False)}")

                mult = int(cp.get('capture_multiplier', 0) or 0)
                if _alive(capture_mult_label):
                    if mult > 0:
                        capture_mult_label.set_text(f"x{mult}")
                    else:
                        capture_mult_label.set_text('')


                new_events = state.get('events', [])
                if new_events != last_events:
                    last_events = new_events

                # Keep toggles synced if backend state changes elsewhere
                await sync_manual_toggles_from_backend()

                await _asyncio.sleep(0.1)
        finally:
            await close_page_session()

    task = _asyncio.create_task(update_ui())
    game_clock.on('disconnect', lambda _e: task.cancel())


# ========================================================================
# SETTINGS PAGE
# ========================================================================

@ui.page('/settings')
async def settings_ui(from_page: str = '', admin: str = ''):
    ui.colors(primary='#1976D2')
    came_from_mobile = (from_page == 'mobile')

    num_to_mode = {0: 'KOTH', 1: 'AD', 2: 'CP'}
    mode_to_num = {'KOTH': 0, 'AD': 1, 'CP': 2}

    with ui.column().classes('w-full p-8 max-w-2xl mx-auto'):
        ui.label('Game Settings').classes('text-3xl font-bold mb-4')
        def go_back():
            if came_from_mobile:
                target = f'/mobile?admin={admin}' if admin else '/mobile'
                ui.navigate.to(target)
            else:
                ui.navigate.to('/')

        ui.button('← Back to Game', on_click=go_back).classes('mb-4')

        mode_select = ui.select(
            ['KOTH', 'AD', 'CP'],
            label='Game Mode',
            value='KOTH',
        ).classes('w-full')

        time_limit = ui.number('Time Limit (seconds)', value=60, min=10, max=600).classes('w-full')
        capture_time = ui.number('Capture Time (seconds)', value=20, min=5, max=120).classes('w-full')
        button_threshold = ui.number('Button Threshold (seconds)', value=5, min=1, max=30).classes('w-full')
        start_delay = ui.number('Start Delay (seconds)', value=5, min=0, max=60).classes('w-full')

        async def load_settings_from_backend():
            try:
                async with aiohttp.ClientSession() as session:
                    async with session.get('http://localhost:8080/api/settings/load') as resp:
                        data = await resp.json()
            except Exception as e:
                print(f"[settings] load failed: {e}")
                return

            if data.get('status') == 'not_found':
                return

            backend_mode_num = data.get('mode', 0)
            mode_str = num_to_mode.get(backend_mode_num, 'KOTH')
            mode_select.value = mode_str
            mode_select.update()

            time_limit.value = data.get('time_limit_seconds', 60)
            capture_time.value = data.get('capture_seconds', 20)
            button_threshold.value = data.get('capture_button_threshold_seconds', 5)
            start_delay.value = data.get('start_delay_seconds', 5)

            time_limit.update()
            capture_time.update()
            button_threshold.update()
            start_delay.update()

        async def save_settings():
            current_mode = mode_select.value
            payload = {
                'mode': mode_to_num[current_mode],
                'time_limit_seconds': int(time_limit.value),
                'capture_seconds': int(capture_time.value),
                'capture_button_threshold_seconds': int(button_threshold.value),
                'start_delay_seconds': int(start_delay.value),
            }

            async with aiohttp.ClientSession() as session:
                await session.post('http://localhost:8080/api/configure', json=payload)
                await session.post('http://localhost:8080/api/settings/save')

            ui.notify(f'Settings for {current_mode} saved!')
            await load_settings_from_backend()

        ui.button('Save Settings', on_click=save_settings).props('color=primary').classes('mt-4')

    await load_settings_from_backend()


# ========================================================================
# DEBUG PAGE
# ========================================================================

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
            manual_switch = ui.switch('Manual Proximity Control (use Red/Blue toggles on main screen)')
            manual_status = ui.label('').classes('text-sm text-gray-400')
            attach_sound_opt_in()

        # BLE SCANNER CARD
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full justify-between items-center mb-4'):
                ui.label('🔵 Bluetooth LE Scanner').classes('text-2xl font-bold')
                with ui.row().classes('gap-2'):
                    start_scan_btn = ui.button('▶ START SCAN').props('color=green')
                    stop_scan_btn = ui.button('⏹ STOP').props('color=red')
                    stop_scan_btn.set_visibility(False)

            ble_status = ui.label('Status: Not scanning').classes('text-lg mb-2')
            device_count_label = ui.label('Tracking 0 device(s)').classes('text-md text-gray-400 mb-4')

            ble_columns = [
                {'name': 'name', 'label': 'Device Name', 'field': 'name', 'align': 'left', 'sortable': True},
                {'name': 'rssi', 'label': 'RSSI (dBm)', 'field': 'rssi', 'align': 'center', 'sortable': True},
                {'name': 'address', 'label': 'Address', 'field': 'address', 'align': 'left'},
                {'name': 'last_seen', 'label': 'Last Seen (ms ago)', 'field': 'last_seen_ms', 'align': 'center', 'sortable': True},
                {'name': 'mfg_data', 'label': 'Manufacturer Data', 'field': 'manufacturer_data', 'align': 'left'},
            ]
            ble_table = ui.table(columns=ble_columns, rows=[], row_key='address').classes('w-full')
            ble_table.add_slot('body-cell-rssi', '''
                <q-td :props="props">
                    <q-badge :color="props.value > -60 ? 'green' : props.value > -80 ? 'orange' : 'red'">
                        {{ props.value }}
                    </q-badge>
                </q-td>
            ''')

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
    ui.run_javascript(f"""
    (function() {{
        const el = document.getElementById('{event_box_id}');
        if (!el) return;
        el.addEventListener('scroll', () => {{
            el.dataset.userScroll = (el.scrollTop > 5) ? '1' : '';
        }});
    }})();
    """)

    session_holder = {'session': None}
    last_events: list[str] = []

    async def _get_session():
        if session_holder['session'] is None:
            session_holder['session'] = aiohttp.ClientSession()
        return session_holder['session']

    async def _cleanup(_msg):
        if session_holder['session'] is not None:
            await session_holder['session'].close()
            session_holder['session'] = None

    event_box.on('disconnect', _cleanup)

    async def sync_manual_from_backend():
        try:
            s = await _get_session()
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
            s = await _get_session()
            enabled = bool(manual_switch.value)
            await s.post(f'http://localhost:8080/api/manual/mode/{str(enabled).lower()}')
            manual_status.set_text(
                f"Manual control is {'ON' if enabled else 'OFF'} "
                "(BLE scanning still runs; in manual mode BLE does not change proximity)."
            )
            ui.notify(f"Manual control {'enabled' if enabled else 'disabled'}", type='info')
        except Exception as ex:
            print(f"[debug] manual toggle failed: {ex}")

    manual_switch.on('update:model-value', on_manual_change)

    async def start_ble_scan():
        s = await _get_session()
        await s.post('http://localhost:8080/api/bluetooth/start')
        start_scan_btn.set_visibility(False)
        stop_scan_btn.set_visibility(True)
        ble_status.set_text('Status: Scanning...')
        ble_status.classes('text-green-400', remove='text-gray-400')
        ui.notify('BLE scanning started', type='positive')

    async def stop_ble_scan():
        s = await _get_session()
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

        s = await _get_session()

        try:
            async with s.get('http://localhost:8080/api/state') as resp:
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
                            ui.label(line).classes('text-sm text-gray-900 dark:text-gray-100')
                    last_events[:] = events

                    await ui.run_javascript(f"""
                    (function(){{
                        const el = document.getElementById('{event_box_id}');
                        if (!el) return;
                        if (!el.dataset.userScroll) {{
                            el.scrollTop = 0;
                        }}
                    }})();
                    """)

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

    ui.timer(0.5, update_debug_once, once=False)

ADMIN_PASSWORD = 'battleadmin'  # change this to whatever you want

@ui.page('/mobile')
async def mobile_ui(admin: str = ''):
    is_admin = (admin == ADMIN_PASSWORD)
    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <style>
      html, body, #app {
        margin: 0;
        padding: 0;
        background: #000;
        height: 100%;
        overflow-x: hidden;
        overflow-y: auto;
      }

      .bp-m-root {
        width: 100vw;
        height: 90dvh;
        display: flex;
        flex-direction: column;
        background: #000;
        color: #fff;
        box-sizing: border-box;
      }

      /* ── TOP BAR ── */
      .bp-m-topbar {
        display: flex;
        flex-wrap: wrap;
        align-items: center;
        gap: 0.9rem;  /* more gap between clusters */
        padding: 0.4rem 0.8rem 0.25rem 0.8rem;
        background: #000;
        box-sizing: border-box;
      }
      .bp-m-title {
        font-weight: 700;
        font-size: 1.05rem;
        margin-right: 0.4rem;
      }
      .bp-m-btn-flat {
        font-size: 0.7rem;
        padding: 0 0.5rem;
        min-height: 22px;
      }
      .bp-m-btn-main {
        font-size: 0.8rem;
        padding: 0 0.7rem;
        min-height: 26px;
      }

      /* ── MAIN COLUMN ── */
      .bp-m-main {
        flex: 1 1 auto;
        display: flex;
        flex-direction: column;
        align-items: stretch;
        gap: 0.4rem;
        padding: 0.15rem 0.8rem 0.15rem 0.8rem;
        box-sizing: border-box;
      }

      .bp-m-clock {
        text-align: center;
        font-weight: 300;
        line-height: 1;
        /* noticeably bigger: */
        font-size: clamp(4rem, 6rem, 8rem);
        margin: 1.2rem ;
      }

      .bp-m-bar-label {
        font-size: 0.65rem;
        color: #aaa;
        letter-spacing: 0.06em;
        margin-bottom: 0.08rem;
      }

      .bp-m-capture-shell,
      .bp-m-owner-shell {
        width: 100%;
        height: 32px;  /* same height */
        background: #222;
        border-radius: 12px;
        overflow: hidden;
        position: relative;
      }

      .bp-m-capture-fill,
      .bp-m-owner-fill {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 0%;
        transition: width 0.15s linear;
      }
      .bp-m-capture-fill { background: #6600ff; }
      .bp-m-owner-fill   { background: #ff0000; }

      .bp-m-status-row {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 0.45rem;
        font-size: 0.75rem;
        color: #ccc;
        margin: 0.25rem 0 0.15rem 0;
      }

      /* ── RED / BLU METERS ── */
      .bp-m-meters-row {
        flex: 0 0 auto;                    
        display: flex;
        flex-direction: row;
        gap: 0.75rem;
        align-items: stretch;
        justify-content: center;
        padding: 0.2rem 0 0.1rem 0;
        box-sizing: border-box;
        margin: 0.5rem;
      }

      .bp-m-team-meter {
        flex: 1 1 0;
        max-width: 50%;
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 0.18rem;
      }

      .bp-m-team-label {
        font-size: 0.75rem;
        font-weight: 700;
      }

      .bp-m-vert-shell {
        flex: 0 0 auto;
        height: clamp(64px, 20vh, 40vh);                    
        width: 100%;
        max-width: 160px;
        margin: 0 auto;                    
        background: #222;
        border-radius: 18px;
        position: relative;
        overflow: hidden;
      }

      .bp-m-vert-fill {
        position: absolute;
        bottom: 0;
        left: 0;
        width: 100%;
        height: 0%;
        transition: height 0.15s linear;
      }

      /* ── ADMIN TOGGLES AT BOTTOM ── */
      .bp-m-bottom-admin {
        flex: 0 0 auto;
        width: 100%;
        padding: 0.25rem 0.8rem 0.45rem 0.8rem;
        box-sizing: border-box;
        display: flex;
        justify-content: center;
        gap: 0.6rem;
      }
      .bp-m-toggle {
        font-size: 0.6rem;
      }
    </style>
    """)

    with ui.element('div').classes('bp-m-root') as root:

        # ───────── TOP BAR ─────────
        with ui.element('div').classes('bp-m-topbar'):
            ui.label('BattlePoint').classes('bp-m-title')
            attach_sound_opt_in()
            if is_admin:
                with ui.row().classes('gap-0.6 items-center'):
                    start_btn = ui.button('START').props(
                        'color=green unelevated'
                    ).classes('bp-m-btn-main')
                    stop_btn = ui.button('STOP').props(
                        'color=orange unelevated'
                    ).classes('bp-m-btn-main')
                    stop_btn.set_visibility(False)

                with ui.row().classes('gap-0.9 items-center'):
                    ui.button(
                        'SETTINGS',
                        on_click=lambda: ui.navigate.to(
                            f'/settings?from_page=mobile&admin={ADMIN_PASSWORD}'
                        ),
                    ).props('flat color=white dense').classes('bp-m-btn-flat')

                    ui.button(
                        'DEBUG',
                        on_click=lambda: ui.navigate.to(
                            f'/debug?from_page=mobile&admin={ADMIN_PASSWORD}'
                        ),
                    ).props('flat color=white dense').classes('bp-m-btn-flat')

        # ───────── MAIN CONTENT ─────────
        with ui.element('div').classes('bp-m-main'):

            # Big clock
            game_clock = ui.label('0:00').classes('bp-m-clock')

            # CAPTURE
            with ui.column().classes('w-full'):
                ui.label('CAPTURE').classes('bp-m-bar-label')
                with ui.element('div').classes('bp-m-capture-shell'):
                    capture_fill = ui.element('div').classes('bp-m-capture-fill')

            # Slight spacer
            ui.separator().style('opacity:0; height:6px;')

            # OWNERSHIP
            with ui.column().classes('w-full'):
                ui.label('OWNERSHIP').classes('bp-m-bar-label')
                with ui.element('div').classes('bp-m-owner-shell'):
                    owner_fill = ui.element('div').classes('bp-m-owner-fill')

            # Status text
            with ui.element('div').classes('bp-m-status-row'):
                status_label = ui.label('Waiting...')
                cp_owner = ui.label('Owner: ---')
                cp_capturing = ui.label('Capturing: ---')
                cp_contested = ui.label('Contested: False')

            # BLU (left) and RED (right) meters
            with ui.element('div').classes('bp-m-meters-row'):

                with ui.element('div').classes('bp-m-team-meter'):
                    ui.label('BLU').classes('bp-m-team-label text-blue-400')
                    with ui.element('div').classes('bp-m-vert-shell'):
                        red_fill = ui.element('div').classes('bp-m-vert-fill').style(
                            'background:#ff0000;'
                        )

                with ui.element('div').classes('bp-m-team-meter'):
                    ui.label('RED').classes('bp-m-team-label text-red-400')
                    with ui.element('div').classes('bp-m-vert-shell'):
                        blue_fill = ui.element('div').classes('bp-m-vert-fill').style(
                            'background:#0066ff;'
                        )


        # ───────── BOTTOM ADMIN TOGGLES ─────────
        if is_admin:
            with ui.element('div').classes('bp-m-bottom-admin'):
                blue_toggle = ui.toggle(
                    ['B OFF', 'B ON'], value='B OFF',
                ).props(
                    'dense push toggle-color=blue color=grey-8 text-color=white'
                ).classes('bp-m-toggle')

                red_toggle = ui.toggle(
                    ['R OFF', 'R ON'], value='R OFF',
                ).props(
                    'dense push toggle-color=red color=grey-8 text-color=white'
                ).classes('bp-m-toggle')



    # ───────── HTTP SESSION ─────────
    page_session = {'session': None}

    async def get_page_session() -> aiohttp.ClientSession:
        s = page_session.get('session')
        if s is None or s.closed:
            s = aiohttp.ClientSession()
            page_session['session'] = s
        return s

    async def close_page_session():
        s = page_session.get('session')
        if s and not s.closed:
            await s.close()
        page_session['session'] = None

    # ───────── ADMIN HANDLERS ─────────
    if is_admin:

        async def start_game_click(_e=None):
            s = await get_page_session()
            await s.post('http://localhost:8080/api/start')

        async def stop_game_click(_e=None):
            s = await get_page_session()
            await s.post('http://localhost:8080/api/stop')

        start_btn.on('click', start_game_click)
        stop_btn.on('click', stop_game_click)

        async def _handle_team_toggle(team: str, toggle, on_label: str):
            s = await get_page_session()
            new_val = (toggle.value == on_label)
            endpoint = 'red' if team == 'red' else 'blu'
            await s.post(
                f'http://localhost:8080/api/manual/{endpoint}/{str(new_val).lower()}'
            )

        async def on_red_toggle(_e):
            await _handle_team_toggle('red', red_toggle, 'R ON')

        async def on_blue_toggle(_e):
            await _handle_team_toggle('blu', blue_toggle, 'B ON')

        red_toggle.on('update:model-value', on_red_toggle)
        blue_toggle.on('update:model-value', on_blue_toggle)

    # ───────── POLLING LOOP ─────────

    def _alive(el) -> bool:
        try:
            _ = el.client
            return True
        except RuntimeError:
            return False

    async def update_ui():
        try:
            while _alive(game_clock):
                try:
                    s = await get_page_session()
                    async with s.get('http://localhost:8080/api/state') as resp:
                        state = await resp.json()
                except Exception:
                    await asyncio.sleep(0.4)
                    continue

                phase = state.get('phase', 'idle')
                running = bool(state.get('running', False))
                winner = state.get('winner')

                # Clock + status
                if phase == 'countdown':
                    cd = int(state.get('countdown_remaining', 0) or 0)
                    if _alive(game_clock):
                        game_clock.set_text(f'{cd}')
                    if _alive(status_label):
                        status_label.set_text('COUNTDOWN...')
                else:
                    rem = int(state.get('remaining_seconds', 0) or 0)
                    if _alive(game_clock):
                        game_clock.set_text(f'{rem // 60}:{rem % 60:02d}')
                    if _alive(status_label):
                        if running:
                            status_label.set_text('GAME RUNNING')
                        elif winner:
                            status_label.set_text(f'Winner: {winner}')
                        else:
                            status_label.set_text('Waiting...')

                meters = state.get('meters', {})

                # BLU vertical (left)
                m2 = meters.get('timer2')
                if m2 and _alive(blue_fill):
                    blue_fill.style(
                        f'height:{m2.get("percent", 0)}%;'
                        f'background:{m2.get("fg", "#0066ff")};'
                        'transition:height 0.15s linear;'
                    )

                # RED vertical (right)
                m1 = meters.get('timer1')
                if m1 and _alive(red_fill):
                    red_fill.style(
                        f'height:{m1.get("percent", 0)}%;'
                        f'background:{m1.get("fg", "#ff0000")};'
                        'transition:height 0.15s linear;'
                    )

                # Capture bar
                m_cap = meters.get('capture')
                if m_cap and _alive(capture_fill):
                    pct = m_cap.get('percent', 0)
                    fg = m_cap.get('fg', '#6600ff')
                    capture_fill.style(
                        f'width:{pct}%; background:{fg}; '
                        'transition:width 0.15s linear;'
                    )

                # Ownership bar
                m_owner = meters.get('owner')
                if m_owner and _alive(owner_fill):
                    pct = m_owner.get('percent', 50)
                    fg = m_owner.get('fg') or '#ff0000'
                    if fg.lower() in ('#000000', 'black'):
                        pct = 50
                        fg = '#000000'
                    owner_fill.style(
                        f'width:{pct}%; background:{fg}; '
                        'transition:width 0.15s linear;'
                    )

                # CP labels
                cp = state.get('control_point', {})
                if _alive(cp_owner):
                    cp_owner.set_text(f'Owner: {cp.get("owner", "---")}')
                if _alive(cp_capturing):
                    cp_capturing.set_text(f'Capturing: {cp.get("capturing", "---")}')
                if _alive(cp_contested):
                    cp_contested.set_text(f'Contested: {cp.get("contested", False)}')

                # Admin START/STOP visibility
                if is_admin:
                    if (phase in ('running', 'countdown')) or running:
                        start_btn.set_visibility(False)
                        stop_btn.set_visibility(True)
                    else:
                        start_btn.set_visibility(True)
                        stop_btn.set_visibility(False)

                await asyncio.sleep(0.2)
        finally:
            await close_page_session()

    task = asyncio.create_task(update_ui())
    root.on('disconnect', lambda _e: task.cancel())




# ========================================================================
# ENTRYPOINT
# ========================================================================

if __name__ in {"__main__", "__mp_main__"}:
    async def start_game_loop():
        global _game_loop_task
        if _game_loop_task is None:
            _game_loop_task = asyncio.create_task(game_loop())

    app.on_startup(start_game_loop)
    ui.run(title='BattlePoint - TF2 Game', port=8080, storage_secret='battlepoint_secret', reload=False)
