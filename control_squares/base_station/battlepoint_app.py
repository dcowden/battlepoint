"""
BattlePoint - Real-life Team Fortress 2 Game
Python/NiceGUI Implementation with BLE Scanner
"""

from enum import Enum
from dataclasses import dataclass
from typing import Optional, List, Callable, Dict
from battlepoint_core import Clock, Team, BluetoothTag, TagType, EventManager, Team, TeamColor, Proximity, GameOptions, LedMeter,GameMode,team_text,RealClock,ControlPoint
from battlepoint_game import create_game,BaseGame
import asyncio
import os
import json
import random
import pygame
import aiohttp
import sys
import time
import threading
from bleak import BleakScanner
from nicegui import ui, app

# Windows-specific fix
if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())


# ========================================================================
# ENHANCED BLE SCANNER (merged from ble_scanner.py)
# ========================================================================

class EnhancedBLEScanner:
    """
    BLE scanner that:
    - On Windows: runs Bleak in its *own* event loop inside a background thread
      (this matches your working CLI pattern and avoids the "Thread is configured
       for Windows GUI but callbacks are not working." crash from HTTP handlers)
    - On Linux: just scans in the main asyncio loop like usual.
    """

    def __init__(self, clock):
        self.clock = clock
        self.devices: Dict[str, dict] = {}

        # runtime state
        self._scanner: Optional[BleakScanner] = None
        self._scanning: bool = False
        self._want_scan: bool = False  # windows thread will watch this
        self._lock = threading.Lock()

        self._is_windows = (sys.platform == "win32")

        # windows-only thread + loop
        self._thread: Optional[threading.Thread] = None
        self._thread_loop: Optional[asyncio.AbstractEventLoop] = None

        if self._is_windows:
            # start the scanner thread immediately so endpoints can just set flags
            self._start_windows_thread()

    # ─────────────────────────────────────────────
    # WINDOWS THREAD SETUP
    # ─────────────────────────────────────────────
    def _start_windows_thread(self):
        if self._thread is not None:
            return

        def _runner():
            # this is now the scanner thread
            # IMPORTANT: set event loop policy here too
            asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            self._thread_loop = loop
            loop.run_until_complete(self._windows_scanner_main())

        self._thread = threading.Thread(target=_runner, daemon=True)
        self._thread.start()
        print("[BLE] Windows scanner thread started")

    async def _windows_scanner_main(self):
        """
        Runs forever in the scanner thread. Watches _want_scan and starts/stops
        the real Bleak scanner from *here*, not from HTTP handlers.
        """
        self._scanner = BleakScanner(detection_callback=self._detection_callback)

        while True:
            try:
                if self._want_scan and not self._scanning:
                    try:
                        await self._scanner.start()
                        self._scanning = True
                        print("[BLE] (win) started scanning")
                    except Exception as e:
                        print(f"[BLE] (win) error starting scan: {e!r}")
                        self._scanning = False
                        # don't spin too hard
                        await asyncio.sleep(1.0)

                elif not self._want_scan and self._scanning:
                    try:
                        await self._scanner.stop()
                    except Exception as e:
                        print(f"[BLE] (win) error stopping scan: {e!r}")
                    self._scanning = False
                    print("[BLE] (win) stopped scanning")

            except Exception as e:
                # never let this loop die
                print(f"[BLE] (win) main loop error: {e!r}")

            await asyncio.sleep(0.2)

    # ─────────────────────────────────────────────
    # COMMON CALLBACK
    # ─────────────────────────────────────────────
    def _detection_callback(self, device, advertisement_data):
        name = device.name
        if not name:
            return

        address = device.address
        rssi = advertisement_data.rssi
        current_time = time.time()

        mfg_data_str = "N/A"
        if advertisement_data.manufacturer_data:
            for _, data_bytes in advertisement_data.manufacturer_data.items():
                try:
                    mfg_data_str = data_bytes.decode('ascii')
                except UnicodeDecodeError:
                    mfg_data_str = data_bytes.hex()
                break

        with self._lock:
            self.devices[address] = {
                'name': name,
                'rssi': rssi,
                'address': address,
                'manufacturer_data': mfg_data_str,
                'last_seen': current_time,
            }

    # ─────────────────────────────────────────────
    # PUBLIC API (called from FastAPI / NiceGUI)
    # ─────────────────────────────────────────────
    async def start_scanning(self):
        """
        Called from your endpoint.
        On Windows: just set a flag; real start happens in the scanner thread.
        On Linux: actually start Bleak here.
        """
        if self._is_windows:
            self._want_scan = True
            return

        # non-windows: do the simple thing
        if self._scanning:
            return
        try:
            self._scanner = BleakScanner(detection_callback=self._detection_callback)
            await self._scanner.start()
            self._scanning = True
            print("[BLE] (linux) scanning started")
        except Exception as e:
            print(f"[BLE] (linux) error starting scan: {e!r}")

    async def stop_scanning(self):
        """
        Called from your endpoint.
        """
        if self._is_windows:
            self._want_scan = False
            return

        if not self._scanning or not self._scanner:
            return
        try:
            await self._scanner.stop()
        finally:
            self._scanning = False
            self._scanner = None
            print("[BLE] (linux) scanning stopped")

    # ─────────────────────────────────────────────
    # READ METHODS
    # ─────────────────────────────────────────────
    def get_devices_summary(self) -> dict:
        with self._lock:
            current_time = time.time()
            device_list = []
            for address, info in self.devices.items():
                device_list.append({
                    'name': info['name'],
                    'rssi': info['rssi'],
                    'address': info['address'],
                    'last_seen_ms': int((current_time - info['last_seen']) * 1000),
                    'manufacturer_data': info['manufacturer_data'],
                })

        device_list.sort(key=lambda d: d['rssi'], reverse=True)

        return {
            'scanning': (self._want_scan if self._is_windows else self._scanning),
            'device_count': len(device_list),
            'devices': device_list,
        }

    def get_active_tags(self, tag_type=None) -> List:
        # you had this for compatibility – keep as a no-op list
        return []


# ========================================================================
# GAME BACKEND
# ========================================================================

class GamePhase(Enum):
    IDLE = "idle"
    COUNTDOWN = "countdown"
    RUNNING = "running"
    ENDED = "ended"

class GameBackend:
    def __init__(self):
        self.clock = RealClock()
        self.game_options = GameOptions()

        # was: EventManager(self.clock, sound_system=None)
        # make it match your core:
        self.event_manager = EventManager(self.clock)

        self.proximity = Proximity(self.game_options, self.clock)
        self.control_point = ControlPoint(self.event_manager, self.clock)
        self.scanner = EnhancedBLEScanner(self.clock)

        self.owner_meter = LedMeter('owner', 20)
        self.capture_meter = LedMeter('capture', 20)
        self.timer1 = LedMeter('timer1', 20)
        self.timer2 = LedMeter('timer2', 20)

        # >>> MATCH C: idle = dark, zeroed
        self.owner_meter.fgColor(TeamColor.BLACK)
        self.owner_meter.bgColor(TeamColor.BLACK)
        self.owner_meter.setMaxValue(1)
        self.owner_meter.setToMin()

        self.capture_meter.fgColor(TeamColor.BLACK)
        self.capture_meter.bgColor(TeamColor.BLACK)
        self.capture_meter.setMaxValue(1)
        self.capture_meter.setToMin()

        self.timer1.fgColor(TeamColor.BLACK)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer1.setMaxValue(1)
        self.timer1.setToMin()

        self.timer2.fgColor(TeamColor.BLACK)
        self.timer2.bgColor(TeamColor.BLACK)
        self.timer2.setMaxValue(1)
        self.timer2.setToMin()
        # <<<

        self.game: Optional[BaseGame] = None

        # runtime phase
        self._phase = GamePhase.IDLE
        self._running = False
        self._countdown_total = 0
        self._countdown_started_ms = 0
        self._last_announced_second = None

    def configure(self, options: GameOptions):
        options.validate()
        self.game_options = options

    def start_game(self):
        self.game = create_game(self.game_options.mode)

        self.game.init(
            self.control_point,
            self.game_options,
            self.event_manager,
            self.owner_meter,
            self.capture_meter,
            self.timer1,
            self.timer2,
            self.clock,
        )

        self._countdown_total = max(0, int(self.game_options.start_delay_seconds))
        self._countdown_started_ms = self.clock.milliseconds()
        self._last_announced_second = None

        for m in (self.timer1, self.timer2, self.owner_meter, self.capture_meter):
            m.setMaxValue(max(1, self._countdown_total or 1))
            m.setToMax()

        # match C handle_start_game()
        self.owner_meter.fgColor(TeamColor.YELLOW)
        self.owner_meter.bgColor(TeamColor.BLACK)
        self.capture_meter.fgColor(TeamColor.YELLOW)
        self.capture_meter.bgColor(TeamColor.BLACK)
        self.timer1.fgColor(TeamColor.YELLOW)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.YELLOW)
        self.timer2.bgColor(TeamColor.BLACK)

        self.event_manager.starting_game()

        if self._countdown_total > 0:
            self._phase = GamePhase.COUNTDOWN
            self._running = False
        else:
            self._do_start_game_now()

    def _do_start_game_now(self):
        assert self.game is not None

        self.game.start()

        if self.event_manager:
            self.event_manager.game_started()

        self.game.update_display()

        self._phase = GamePhase.RUNNING
        self._running = True

        ss = getattr(self, "sound_system", None)
        if ss:
            ss.stop()

    def stop_game(self):
        if self.game:
            self.game.end()

        self._phase = GamePhase.IDLE
        self._running = False
        self._last_announced_second = None

        # return meters to idle
        self.owner_meter.fgColor(TeamColor.BLACK)
        self.owner_meter.bgColor(TeamColor.BLACK)
        self.owner_meter.setMaxValue(1)
        self.owner_meter.setToMin()

        self.capture_meter.fgColor(TeamColor.BLACK)
        self.capture_meter.bgColor(TeamColor.BLACK)
        self.capture_meter.setMaxValue(1)
        self.capture_meter.setToMin()

        self.timer1.fgColor(TeamColor.BLACK)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer1.setMaxValue(1)
        self.timer1.setToMin()

        self.timer2.fgColor(TeamColor.BLACK)
        self.timer2.bgColor(TeamColor.BLACK)
        self.timer2.setMaxValue(1)
        self.timer2.setToMin()

        self.game = None

    def update(self):
        now_ms = self.clock.milliseconds()

        # COUNTDOWN
        if self._phase == GamePhase.COUNTDOWN:
            elapsed_ms = now_ms - self._countdown_started_ms
            elapsed_s = elapsed_ms // 1000
            remaining = max(0, self._countdown_total - elapsed_s)

            for m in (self.timer1, self.timer2, self.owner_meter, self.capture_meter):
                m.setMaxValue(max(1, self._countdown_total or 1))
                m.setValue(remaining)

            if remaining != self._last_announced_second:
                self.event_manager.starts_in_seconds(remaining)
                self._last_announced_second = remaining

            if remaining <= 0:
                self._do_start_game_now()

            return

        # RUNNING
        if self._phase == GamePhase.RUNNING:
            if self.game:
                self.game.update()

                cp = self.control_point
                cp_pct = cp.get_capture_progress_percent()
                self.capture_meter.setMaxValue(100)
                self.capture_meter.setValue(int(cp_pct))

                if cp.is_contested():
                    self.capture_meter.fgColor(TeamColor.PURPLE)
                    self.capture_meter.bgColor(TeamColor.BLACK)
                else:
                    cap_team = cp.get_capturing()
                    if cap_team == Team.RED:
                        self.capture_meter.fgColor(TeamColor.RED)
                        self.capture_meter.bgColor(TeamColor.BLACK)
                    elif cap_team == Team.BLU:
                        self.capture_meter.fgColor(TeamColor.BLUE)
                        self.capture_meter.bgColor(TeamColor.BLACK)
                    else:
                        owner = cp.get_owner()
                        if owner == Team.RED:
                            self.capture_meter.fgColor(TeamColor.RED)
                            self.capture_meter.bgColor(TeamColor.BLACK)
                        elif owner == Team.BLU:
                            self.capture_meter.fgColor(TeamColor.BLUE)
                            self.capture_meter.bgColor(TeamColor.BLACK)
                        else:
                            self.capture_meter.fgColor('#555555')
                            self.capture_meter.bgColor(TeamColor.BLACK)

                if self.game.is_over():
                    self._phase = GamePhase.ENDED
                    self._running = False

            return

        # ENDED / IDLE – nothing special
        return

    def get_state(self) -> dict:
        meters = {
            'timer1': self.timer1.to_dict(),
            'timer2': self.timer2.to_dict(),
            'owner': self.owner_meter.to_dict(),
            'capture': self.capture_meter.to_dict(),
        }

        events = [ev.to_display() for ev in self.event_manager.get_events(100)]

        # COUNTDOWN
        if self._phase == GamePhase.COUNTDOWN:
            now_ms = self.clock.milliseconds()
            elapsed_ms = now_ms - self._countdown_started_ms
            elapsed_s = elapsed_ms // 1000
            remaining = max(0, self._countdown_total - elapsed_s)
            return {
                'running': False,
                'phase': 'countdown',
                'countdown_remaining': remaining,
                'meters': meters,
                'events': events,
            }

        # RUNNING / ENDED
        if self.game and (self._phase in (GamePhase.RUNNING, GamePhase.ENDED)):
            red_left = self.game.get_remaining_seconds_for_team(Team.RED)
            blu_left = self.game.get_remaining_seconds_for_team(Team.BLU)

            owner = self.control_point.get_owner()
            if owner == Team.BLU:
                remaining_seconds = blu_left
            elif owner == Team.RED:
                remaining_seconds = red_left
            else:
                remaining_seconds = max(red_left, blu_left)

            return {
                'running': self._running,
                'phase': 'running' if self._running else 'ended',
                'remaining_seconds': remaining_seconds,
                'time_limit_seconds': self.game_options.time_limit_seconds,
                'red_remaining_seconds': red_left,
                'blu_remaining_seconds': blu_left,
                'winner': team_text(self.game.get_winner()) if self.game.is_over() else None,
                'control_point': {
                    'owner': team_text(self.control_point.get_owner()),
                    'capturing': team_text(self.control_point.get_capturing()),
                    'on': team_text(self.control_point.get_on()),
                    'contested': self.control_point.is_contested(),
                    'progress': self.control_point.get_capture_progress_percent(),
                },
                'red_accumulated': self.game.get_accumulated_seconds(Team.RED),
                'blu_accumulated': self.game.get_accumulated_seconds(Team.BLU),
                'meters': meters,
                'events': events,
            }

        # IDLE
        return {
            'running': False,
            'phase': 'idle',
            'meters': meters,
            'events': events,
        }


# ============================================================================
# SETTINGS PERSISTENCE
# ============================================================================

class SettingsManager:
    def __init__(self, filename: str = "battlepoint_settings.json"):
        self.filename = filename

    def save_settings(self, options: GameOptions, volume: int = 10, brightness: int = 50) -> bool:
        try:
            data = {
                'mode': options.mode.value,
                'capture_seconds': options.capture_seconds,
                'capture_button_threshold_seconds': options.capture_button_threshold_seconds,
                'time_limit_seconds': options.time_limit_seconds,
                'start_delay_seconds': options.start_delay_seconds,
                'volume': volume,
                'brightness': brightness
            }

            with open(self.filename, 'w') as f:
                json.dump(data, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving settings: {e}")
            return False

    def load_settings(self) -> Optional[tuple[GameOptions, int, int]]:
        if not os.path.exists(self.filename):
            return None

        try:
            with open(self.filename, 'r') as f:
                data = json.load(f)

            options = GameOptions(
                mode=GameMode(data.get('mode', 0)),
                capture_seconds=data.get('capture_seconds', 20),
                capture_button_threshold_seconds=data.get('capture_button_threshold_seconds', 5),
                time_limit_seconds=data.get('time_limit_seconds', 60),
                start_delay_seconds=data.get('start_delay_seconds', 5)
            )

            volume = data.get('volume', 10)
            brightness = data.get('brightness', 50)

            return options, volume, brightness
        except Exception as e:
            print(f"Error loading settings: {e}")
            return None


# ============================================================================
# SOUND SYSTEM
# ============================================================================

class SoundSystem:
    def __init__(self, base_dir: str = "sounds"):
        self.enabled = True
        self.volume = 10
        self.base_dir = base_dir

        try:
            pygame.mixer.init()
            pygame.mixer.music.set_volume(self.volume / 30.0)
            self.ok = True
            print("[SOUND] pygame.mixer initialized")
        except Exception as e:
            self.ok = False
            print(f"[SOUND] ERROR initializing pygame.mixer: {e}")

        self.sound_map = {
            1:  "0001_announcer_alert.mp3",
            2:  "0002_announcer_alert_center_control_being_contested.mp3",
            3:  "0014_announcer_last_flag.mp3",
            4:  "0015_announcer_last_flag2.mp3",
            5:  "0016_announcer_overtime.mp3",
            6:  "0017_announcer_overtime2.mp3",
            7:  "0018_announcer_sd_monkeynaut_end_crash02.mp3",
            8:  "0019_announcer_stalemate.mp3",
            9:  "0020_announcer_success.mp3",
            10: "0021_announcer_time_added.mp3",
            11: "0022_announcer_tournament_started4.mp3",
            12: "0023_announcer_victory.mp3",
            13: "0024_announcer_warning.mp3",
            14: "0025_announcer_we_captured_control.mp3",
            15: "0026_announcer_we_lost_control.mp3",
            16: "0027_announcer_you_failed.mp3",
            17: "0028_engineer_specialcompleted10.mp3",
            18: "0030_gamestartup2.mp3",
            19: "0031_gamestartup4.mp3",
            20: "0032_gamestartup5.mp3",
            21: "0033_gamestartup6.mp3",
            22: "0034_gamestartup7.mp3",
            23: "0035_gamestartup8.mp3",
            24: "0036_gamestartup15.mp3",
            25: "0037_gamestartup16.mp3",
            26: "announcer_begins_10sec.mp3",
            27: "announcer_begins_1sec.mp3",
            28: "announcer_begins_20sec.mp3",
            29: "announcer_begins_2sec.mp3",
            30: "announcer_begins_30sec.mp3",
            31: "announcer_begins_3sec.mp3",
            32: "announcer_begins_4sec.mp3",
            33: "announcer_begins_5sec.mp3",
            34: "announcer_begins_60sec.mp3",
            35: "announcer_ends_10sec.mp3",
            36: "announcer_ends_1sec.mp3",
            37: "announcer_ends_20sec.mp3",
            38: "announcer_ends_2min.mp3",
            39: "announcer_ends_2sec.mp3",
            40: "announcer_ends_30sec.mp3",
            41: "announcer_ends_3sec.mp3",
            42: "announcer_ends_4sec.mp3",
            43: "announcer_ends_5sec.mp3",
            44: "announcer_ends_60sec.mp3",
            45: "announcer_ends_6sec.mp3",
            46: "announcer_ends_7sec.mp3",
            47: "announcer_ends_8sec.mp3",
            48: "announcer_ends_9sec.mp3",
            49: "announcer_time_added.mp3",
        }

        self._menu_tracks = list(range(18, 26))

    def _load_path(self, sound_id: int) -> str | None:
        filename = self.sound_map.get(sound_id)
        if not filename:
            print(f"[SOUND] unknown sound id {sound_id}")
            return None

        path = os.path.join(self.base_dir, filename)
        if not os.path.exists(path):
            print(f"[SOUND] file not found: {path}")
            return None
        return path

    def play(self, sound_id: int):
        if not self.enabled or not self.ok:
            return
        path = self._load_path(sound_id)
        if not path:
            return
        try:
            pygame.mixer.music.load(path)
            pygame.mixer.music.play()
            print(f"[SOUND] Playing {sound_id}: {path}")
        except Exception as e:
            print(f"[SOUND] ERROR playing {path}: {e}")

    def loop(self, sound_id: int):
        if not self.enabled or not self.ok:
            return
        path = self._load_path(sound_id)
        if not path:
            return
        try:
            pygame.mixer.music.load(path)
            pygame.mixer.music.play(loops=-1)
            print(f"[SOUND] Looping {sound_id}: {path}")
        except Exception as e:
            print(f"[SOUND] ERROR looping {path}: {e}")

    def play_menu_track(self):
        if not self.enabled or not self.ok:
            return
        snd_id = random.choice(self._menu_tracks)
        self.loop(snd_id)

    def set_volume(self, volume: int):
        self.volume = max(0, min(30, volume))
        if getattr(self, "ok", False):
            pygame.mixer.music.set_volume(self.volume / 30.0)

    def stop(self):
        if getattr(self, "ok", False):
            pygame.mixer.music.stop()


# ============================================================================
# ENHANCED GAME BACKEND
# ============================================================================
class EnhancedGameBackend(GameBackend):
    """Extended backend with sound, settings, and Bluetooth, MATCHING THE C FLOW"""

    def __init__(self):
        super().__init__()

        # phase/state the base class expects
        self._phase = GamePhase.IDLE
        self._running = False
        self._countdown_total = 0
        self._countdown_started_ms = 0
        self._last_announced_second = None

        # real sound system
        self.sound_system = SoundSystem()

        # IMPORTANT: track whether menu music is already playing
        self._menu_music_on = False

        # re-bind event manager so sound is used
        self.event_manager = EventManager(self.clock, self.sound_system)
        # re-bind control point to that event manager
        self.control_point = ControlPoint(self.event_manager, self.clock)

        # settings + BT
        self.settings_manager = SettingsManager()

        # load saved settings, like the C code loads from EEPROM
        saved = self.settings_manager.load_settings()
        if saved:
            options, volume, brightness = saved
            self.configure(options)
            self.sound_system.set_volume(volume)

        # C: at startup we go to menu music ONCE
        self.sound_system.play_menu_track()
        self._menu_music_on = True

    # --- game lifecycle -------------------------------------------------

    def start_game(self):
        # if we are on menu music, kill it now
        if self._menu_music_on:
            self.sound_system.stop()
            self._menu_music_on = False

        # then do the normal GameBackend start flow
        super().start_game()

    def _do_start_game_now(self):
        # same reason: make sure menu music is off when real game starts
        if self._menu_music_on:
            self.sound_system.stop()
            self._menu_music_on = False
        super()._do_start_game_now()

    def stop_game(self):
        # 1) run the common stop flow (ends game, sets idle, zeroes meters)
        super().stop_game()

        # 2) NOW we can go back to menu music, BUT ONLY IF NOT ALREADY PLAYING
        if not self._menu_music_on:
            self.sound_system.play_menu_track()
            self._menu_music_on = True

    # ─────────────────────────────────────────────
    # BLE SCANNER METHODS
    # ─────────────────────────────────────────────
    async def start_bluetooth_scanning(self):
        # now safe on windows: it only sets a flag
        await self.scanner.start_scanning()

    async def stop_bluetooth_scanning(self):
        await self.scanner.stop_scanning()

    def get_ble_devices_summary(self) -> dict:
        return self.scanner.get_devices_summary()

    # ─────────────────────────────────────────────
    # SETTINGS PASSTHROUGHS (needed by /api/settings/save)
    # ─────────────────────────────────────────────
    def save_settings(self) -> bool:
        # we can also read current volume from the sound system
        volume = getattr(self.sound_system, "volume", 10)
        return self.settings_manager.save_settings(self.game_options, volume=volume, brightness=50)

    def load_settings(self):
        return self.settings_manager.load_settings()

# ============================================================================
# CREATE THE SINGLE BACKEND WE WILL USE EVERYWHERE
# ============================================================================

enhanced_backend = EnhancedGameBackend()

# ============================================================================
# FASTAPI / NICEGUI ENDPOINTS
# ============================================================================

_game_loop_task = None


async def game_loop():
    while True:
        enhanced_backend.update()
        await asyncio.sleep(0.1)


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
        start_delay_seconds=options.get('start_delay_seconds', 5)
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
            "volume": volume
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


# ============================================================================
# BLE SCANNER ENDPOINTS
# ============================================================================

@app.get("/api/bluetooth/devices")
async def get_bluetooth_devices():
    """Get all BLE devices currently being tracked."""
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
                "last_seen": tag.last_seen
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


# ============================================================================
# NICEGUI FRONTEND (main /)
# ============================================================================

from nicegui import ui, app
import asyncio
import aiohttp


@ui.page('/')
async def game_ui():
    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <script>
      window.BP = window.BP || {};
      BP.fallbackBeep = (freq=880, ms=120) => {
        try{
          const a = new (window.AudioContext||window.webkitAudioContext)();
          const o = a.createOscillator(), g = a.createGain();
          o.connect(g); g.connect(a.destination);
          o.type = 'square'; o.frequency.value = freq;
          g.gain.setValueAtTime(0.15, a.currentTime);
          o.start();
          setTimeout(()=>{ g.gain.exponentialRampToValueAtTime(0.0001, a.currentTime+0.01); o.stop(); a.close(); }, ms);
        }catch(e){}
      };
    </script>
    """)

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
        justify-content: space-between;
        padding: 0 1rem;
        gap: 1rem;
      }
      .bp-topbar-left {
        display: flex;
        align-items: center;
        gap: 0.75rem;
      }
      .bp-topbar-center {
        display: flex;
        align-items: center;
        justify-content: center;
        gap: 0.5rem;
      }
      .bp-topbar-right {
        display: flex;
        align-items: center;
        gap: 0.5rem;
      }
      .bp-main {
        height: calc(100vh - 52px - 240px);
        display: flex;
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
      }
      .bp-side {
        width: 20vw;
        min-width: 140px;
        height: 100%;
        display: flex;
        flex-direction: column;
        gap: 0.75rem;
        align-items: center;
        justify-content: center;
      }
      .bp-vert-shell {
        width: 350px;
        height: 100vh;
        max-height: calc(100vh - 52px - 160px);
        background: #222;
        border: 3px solid #444;
        border-radius: 16px;
        position: relative;
        overflow: hidden;
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
        font-size: min(35rem, 35vh);
        line-height: 1;
        font-weight: 700;
        color: #fff;
        text-align: center;
      }
      .bp-status {
        font-size: 4.4rem;
        color: #ddd;
      }
      .bp-capture-shell {
        width: min(60vw, 900px);
        height: 180px;
        background: #222;
        border: 2px solid #444;
        border-radius: 12px;
        overflow: hidden;
        position: relative;
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
        height: 240px;
        display: flex;
        flex-direction: column;
        gap: 0.4rem;
        padding: 0.4rem 0.8rem 0.4rem 0.8rem;
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

        # top bar
        with ui.element('div').classes('bp-topbar'):
            # left
            with ui.element('div').classes('bp-topbar-left'):
                ui.label('BattlePoint').classes('text-white text-lg font-bold')

            # center – moved buttons here
            with ui.element('div').classes('bp-topbar-center'):
                ui.button('Red', on_click=lambda: simulate_red()).props('color=red')
                ui.button('Blue', on_click=lambda: simulate_blue()).props('color=blue')
                ui.button('Start', on_click=lambda: start_game()).props('color=green')
                ui.button('Stop', on_click=lambda: stop_game()).props('color=orange')

            # right – existing nav
            with ui.element('div').classes('bp-topbar-right'):
                ui.button('Game', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.button('Settings', on_click=lambda: ui.navigate.to('/settings')).props('flat color=white')
                ui.button('Debug', on_click=lambda: ui.navigate.to('/debug')).props('flat color=white')

        # main
        with ui.element('div').classes('bp-main'):

            # LEFT: timer1 -> (KOTH: BLUE)
            with ui.element('div').classes('bp-side'):
                ui.label('BLUE').classes('text-4xl font-bold text-blue-400')
                with ui.element('div').classes('bp-vert-shell'):
                    red_fill = ui.element('div').classes('bp-vert-fill')

            # CENTER
            with ui.element('div').classes('bp-center'):
                game_clock = ui.label('0:00').classes('bp-clock')
                status_label = ui.label('Waiting...').classes('bp-status')

                # WIDE CAPTURE BAR
                with ui.element('div').classes('bp-capture-shell'):
                    capture_fill = ui.element('div').classes('bp-capture-fill')

                cp_owner = ui.label('Owner: ---').classes('text-white')
                cp_capturing = ui.label('Capturing: ---').classes('text-white')
                cp_contested = ui.label('Contested: False').classes('text-white')

            # RIGHT: timer2 -> (KOTH: RED)
            with ui.element('div').classes('bp-side'):
                ui.label('RED').classes('text-4xl font-bold text-red-400')
                with ui.element('div').classes('bp-vert-shell'):
                    blue_fill = ui.element('div').classes('bp-vert-fill')

        # bottom: owner meter only now
        with ui.element('div').classes('bp-bottom'):
            with ui.element('div').classes('bp-bottom-wrapper'):
                with ui.element('div').classes('bp-horiz-shell'):
                    horiz_red = ui.element('div').classes('bp-horiz-red')

    # helpers
    async def simulate_red():
        await ui.run_javascript('fetch("/api/proximity/red", {method: "POST"})')

    async def simulate_blue():
        await ui.run_javascript('fetch("/api/proximity/blue", {method: "POST"})')

    async def start_game():
        await ui.run_javascript('fetch("/api/start", {method: "POST"})')

    async def stop_game():
        await ui.run_javascript('fetch("/api/stop", {method: "POST"})')

    async def update_ui():
        session: aiohttp.ClientSession | None = None
        last_events: list[str] = []

        while True:
            try:
                if session is None:
                    session = aiohttp.ClientSession()

                async with session.get('http://localhost:8080/api/state') as resp:
                    state = await resp.json()
            except Exception:
                await asyncio.sleep(0.3)
                continue

            phase = state.get('phase', 'idle')
            meters = state.get('meters', {})

            m1 = meters.get('timer1')
            if m1:
                red_fill.style(f'height: {m1.get("percent", 0)}%; background: {m1.get("fg", "#0000FF")};')

            m2 = meters.get('timer2')
            if m2:
                blue_fill.style(f'height: {m2.get("percent", 0)}%; background: {m2.get("fg", "#FF0000")};')

            m_cap = meters.get('capture')
            if m_cap:
                pct = m_cap.get('percent', 0)
                fg = m_cap.get('fg', '#6600ff')
                capture_fill.style(f'width: {pct}%; background: {fg};')

            # BOTTOM OWNER BAR
            m_owner = meters.get('owner')
            if m_owner:
                pct = m_owner.get("percent", 50)
                fg = m_owner.get("fg") or "#FF0000"

                if fg.lower() in ("#000000", "black"):
                    pct = 50
                    fg = "#000000"

                horiz_red.style(f'width: {pct}%; background: {fg};')

            # CLOCK / PHASE
            if phase == 'countdown':
                cd = state.get('countdown_remaining', 0)
                game_clock.set_text(f'{cd}')
                status_label.set_text('COUNTDOWN...')
            else:
                rem = state.get('remaining_seconds', 0)
                game_clock.set_text(f'{rem // 60}:{rem % 60:02d}')
                if state.get('running'):
                    status_label.set_text('GAME RUNNING')
                elif state.get('winner'):
                    status_label.set_text(f"Winner: {state['winner']}")
                else:
                    status_label.set_text('Waiting...')

            # CONTROL POINT TEXT
            cp = state.get('control_point', {})
            cp_owner.set_text(f"Owner: {cp.get('owner', '---')}")
            cp_capturing.set_text(f"Capturing: {cp.get('capturing', '---')}")
            cp_contested.set_text(f"Contested: {cp.get('contested', False)}")

            # EVENTS
            new_events = state.get('events', [])
            if new_events != last_events:
                last_events = new_events

            await asyncio.sleep(0.1)

    ui.timer(0.1, update_ui, once=False)


# ============================================================================
# SETTINGS PAGE
# ============================================================================

@ui.page('/settings')
async def settings_ui():
    ui.colors(primary='#1976D2')

    # backend <-> UI mapping
    num_to_mode = {0: 'KOTH', 1: 'AD', 2: 'CP'}
    mode_to_num = {'KOTH': 0, 'AD': 1, 'CP': 2}

    with ui.column().classes('w-full p-8 max-w-2xl mx-auto'):
        ui.label('Game Settings').classes('text-3xl font-bold mb-4')
        ui.button('← Back to Game', on_click=lambda: ui.navigate.to('/')).classes('mb-4')

        # temp defaults – will be overwritten by load() below
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
            import aiohttp
            try:
                async with aiohttp.ClientSession() as session:
                    async with session.get('http://localhost:8080/api/settings/load') as resp:
                        data = await resp.json()
            except Exception as e:
                print(f"[settings] load failed: {e}")
                return

            # backend can return {"status": "not_found"}
            if data.get('status') == 'not_found':
                return

            # mode
            backend_mode_num = data.get('mode', 0)
            mode_str = num_to_mode.get(backend_mode_num, 'KOTH')
            mode_select.set_value(mode_str)

            # numbers
            time_limit.set_value(data.get('time_limit_seconds', 60))
            capture_time.set_value(data.get('capture_seconds', 20))
            button_threshold.set_value(data.get('capture_button_threshold_seconds', 5))
            start_delay.set_value(data.get('start_delay_seconds', 5))

        async def save_settings():
            import aiohttp

            current_mode = mode_select.value
            payload = {
                'mode': mode_to_num[current_mode],
                'time_limit_seconds': int(time_limit.value),
                'capture_seconds': int(capture_time.value),
                'capture_button_threshold_seconds': int(button_threshold.value),
                'start_delay_seconds': int(start_delay.value),
            }

            async with aiohttp.ClientSession() as session:
                # update running backend
                await session.post('http://localhost:8080/api/configure', json=payload)
                # persist to disk
                await session.post('http://localhost:8080/api/settings/save')

            ui.notify(f'Settings for {current_mode} saved!')

            # re-load to confirm what backend actually stored
            await load_settings_from_backend()

        ui.button('Save Settings', on_click=save_settings).props('color=primary').classes('mt-4')

    # AFTER building UI: pull current settings from backend
    await load_settings_from_backend()



# ============================================================================
# DEBUG PAGE (ENHANCED WITH BLE SCANNER)
# ============================================================================

@ui.page('/debug')
def debug_ui():
    from nicegui import ui
    ui.colors(primary='#1976D2')

    # ---------- LAYOUT ----------
    with ui.column().classes('w-full p-8 max-w-6xl mx-auto gap-4'):
        ui.label('Debug Console').classes('text-3xl font-bold')
        ui.button('← BACK TO GAME', on_click=lambda: ui.navigate.to('/'))

        # ----- BLE SCANNER CARD -----
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full justify-between items-center mb-4'):
                ui.label('🔵 Bluetooth LE Scanner').classes('text-2xl font-bold')
                with ui.row().classes('gap-2'):
                    start_scan_btn = ui.button('▶ START SCAN', on_click=lambda: start_ble_scan()).props('color=green')
                    stop_scan_btn = ui.button('⏹ STOP', on_click=lambda: stop_ble_scan()).props('color=red')
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

        # ----- EVENT LOG CARD -----
        with ui.card().classes('w-full p-4'):
            with ui.row().classes('w-full items-center justify-between mb-2'):
                ui.label('📋 Event Log').classes('text-xl font-bold')
                event_count_label = ui.label('0 events').classes('text-sm text-gray-400')

            # tall, scrollable (fills most of window)
            event_box = ui.column().classes(
                'w-full bg-gray-50 rounded p-2 gap-1 dark:bg-gray-900'
            ).style('height: calc(50vh - 360px); overflow-y: auto;')
            event_box_id = f"bp-event-box-{event_box.id}"
            event_box.props(f'id={event_box_id}')

        # ----- GAME STATE CARD -----
        with ui.card().classes('w-full p-4'):
            ui.label('⚙️ Game State').classes('text-xl font-bold mb-2')
            state_display = ui.json_editor({}).classes('w-full')

    # ---------- JS: remember if user scrolled ----------
    ui.run_javascript(f"""
    (function() {{
        const el = document.getElementById('{event_box_id}');
        if (!el) return;
        el.addEventListener('scroll', () => {{
            el.dataset.userScroll = (el.scrollTop > 5) ? '1' : '';
        }});
    }})();
    """)

    # ---------- ASYNC PLUMBING ----------
    session_holder = {'session': None}
    last_events: list[str] = []

    async def _get_session():
        import aiohttp
        if session_holder['session'] is None:
            session_holder['session'] = aiohttp.ClientSession()
        return session_holder['session']

    # close session when this page/element goes away
    async def _cleanup(_msg):
        if session_holder['session'] is not None:
            await session_holder['session'].close()
            session_holder['session'] = None

    # THIS is the correct way:
    event_box.on('disconnect', _cleanup)

    # ---------- BUTTON HANDLERS ----------
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

    # ---------- POLLER ----------
    async def update_debug_once():
        # page gone
        if state_display.client is None:
            return

        s = await _get_session()

        try:
            # ---- GAME STATE & EVENTS ----
            async with s.get('http://localhost:8080/api/state') as resp:
                state = await resp.json()
                state_display.value = state
                state_display.update()

                # EXPECT: your fixed GameBackend now returns up to 100 events here
                events = state.get('events', [])
                event_count_label.set_text(f'{len(events)} event(s)')

                if events != last_events:
                    event_box.clear()
                    # newest first
                    with event_box:
                        for line in reversed(events):
                            ui.label(line).classes('text-sm text-gray-900 dark:text-gray-100')
                    last_events[:] = events

                    # auto-scroll to top (where newest is) unless user scrolled
                    await ui.run_javascript(f"""
                    (function(){{
                        const el = document.getElementById('{event_box_id}');
                        if (!el) return;
                        if (!el.dataset.userScroll) {{
                            el.scrollTop = 0;
                        }}
                    }})();
                    """)

            # ---- BLE DEVICES ----
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

# ============================================================================
# ENHANCED GAME UI (/game)
# ============================================================================

@ui.page('/game')
async def enhanced_game_ui():
    ui.colors(primary='#1976D2')

    ui.add_head_html('''
        <style>
            .pulse {
                animation: pulse 2s ease-in-out infinite;
            }
            @keyframes pulse {
                0%, 100% { opacity: 1; }
                50% { opacity: 0.5; }
            }
            .flash {
                animation: flash 0.5s ease-in-out;
            }
            @keyframes flash {
                0%, 100% { background-color: transparent; }
                50% { background-color: yellow; }
            }
        </style>
    ''')

    with ui.column().classes('w-full h-screen bg-gray-900 text-white'):
        with ui.row().classes('w-full justify-between items-center p-4 bg-gray-800'):
            ui.label('🎮 BattlePoint').classes('text-3xl font-bold')
            with ui.row().classes('gap-2'):
                ui.button('Settings', on_click=lambda: ui.navigate.to('/settings'), icon='settings').props('flat')
                ui.button('Debug', on_click=lambda: ui.navigate.to('/debug'), icon='bug_report').props('flat')

        with ui.row().classes('w-full flex-grow p-4 gap-4'):
            with ui.column().classes('w-1/5 gap-4'):
                ui.label('RED TEAM').classes('text-2xl font-bold text-red-500 text-center')
                with ui.card().classes('bg-gray-800 p-4'):
                    red_time = ui.label('0:00').classes('text-4xl font-mono text-center')
                    ui.separator()
                    red_progress = ui.linear_progress(0).props('color=red size=30px')
                ui.space()

            with ui.column().classes('w-3/5 gap-4 items-center justify-center'):
                with ui.card().classes('bg-gray-800 p-8 w-full'):
                    game_status = ui.label('Ready to Start').classes('text-xl text-center text-gray-400 mb-4')
                    main_clock = ui.label('0:00').classes('text-9xl font-mono font-bold text-center')

                with ui.card().classes('bg-gray-800 p-6 w-full'):
                    ui.label('⚔️ CONTROL POINT').classes('text-2xl font-bold text-center mb-4')

                    with ui.row().classes('w-full justify-between mb-2'):
                        cp_owner_label = ui.label('Owner: ---').classes('text-lg')
                        cp_capturing_label = ui.label('Capturing: ---').classes('text-lg')

                    cp_progress_bar = ui.linear_progress(0).props('size=20px color=purple').classes('mb-2')
                    cp_status_label = ui.label('Neutral').classes('text-center text-gray-400')

                with ui.row().classes('gap-2 mt-4'):
                    start_btn = ui.button('▶ START GAME', on_click=lambda: start_game()).props('color=green size=lg')
                    stop_btn = ui.button('⏹ STOP', on_click=lambda: stop_game()).props('color=orange size=lg')
                    stop_btn.set_visibility(False)

            with ui.column().classes('w-1/5 gap-4'):
                ui.label('BLUE TEAM').classes('text-2xl font-bold text-blue-500 text-center')
                with ui.card().classes('bg-gray-800 p-4'):
                    blue_time = ui.label('0:00').classes('text-4xl font-mono text-center')
                    ui.separator()
                    blue_progress = ui.linear_progress(0).props('color=blue size=30px reverse')
                ui.space()

        with ui.row().classes('w-full p-4 bg-gray-800'):
            with ui.column().classes('w-full'):
                ui.label('⚖️ TEAM BALANCE').classes('text-center text-gray-400 mb-2')
                seesaw_meter = ui.linear_progress(0.5).props('size=30px').classes('w-full')
                with ui.row().classes('w-full justify-between text-sm text-gray-400 mt-1'):
                    ui.label('◀ RED')
                    ui.label('BLUE ▶')

        with ui.card().classes('fixed bottom-4 right-4 w-80 bg-gray-800 p-4 max-h-60 overflow-auto'):
            ui.label('📋 Event Feed').classes('text-lg font-bold mb-2')
            event_feed = ui.column().classes('gap-1')

    async def start_game():
        import aiohttp
        async with aiohttp.ClientSession() as session:
            await session.post('http://localhost:8080/api/start')
        start_btn.set_visibility(False)
        stop_btn.set_visibility(True)
        game_status.set_text('🎮 GAME IN PROGRESS')
        game_status.classes('text-green-400', remove='text-gray-400')

    async def stop_game():
        import aiohttp
        async with aiohttp.ClientSession() as session:
            await session.post('http://localhost:8080/api/stop')
        start_btn.set_visibility(True)
        stop_btn.set_visibility(False)
        game_status.set_text('Stopped')
        game_status.classes('text-gray-400', remove='text-green-400 text-yellow-400 flash')

    async def update_ui_loop():
        last_events = []
        import aiohttp
        async with aiohttp.ClientSession() as session:
            while True:
                try:
                    async with session.get('http://localhost:8080/api/state') as resp:
                        state = await resp.json()

                    mins = state.get('remaining_seconds', 0) // 60
                    secs = state.get('remaining_seconds', 0) % 60
                    main_clock.set_text(f'{mins}:{secs:02d}')

                    red_mins = state.get('red_accumulated', 0) // 60
                    red_secs = state.get('red_accumulated', 0) % 60
                    red_time.set_text(f'{red_mins}:{red_secs:02d}')

                    blue_mins = state.get('blu_accumulated', 0) // 60
                    blue_secs = state.get('blu_accumulated', 0) % 60
                    blue_time.set_text(f'{blue_mins}:{blue_secs:02d}')

                    cp = state.get('control_point', {})
                    cp_owner_label.set_text(f'Owner: {cp.get("owner", "---")}')
                    cp_capturing_label.set_text(f'Capturing: {cp.get("capturing", "---")}')
                    cp_progress_bar.set_value(cp.get('progress', 0) / 100)

                    if cp.get('contested'):
                        cp_status_label.set_text('⚠️ CONTESTED!')
                        cp_status_label.classes('text-yellow-400 pulse')
                    else:
                        cp_status_label.set_text(f'Status: {cp.get("owner", "---")}')
                        cp_status_label.classes('text-gray-400', remove='text-yellow-400 pulse')

                    total = state.get('red_accumulated', 0) + state.get('blu_accumulated', 0)
                    if total > 0:
                        balance = state.get('blu_accumulated', 0) / total
                    else:
                        balance = 0.5
                    seesaw_meter.set_value(balance)

                    new_events = state.get('events', [])
                    if new_events != last_events:
                        event_feed.clear()
                        for event in reversed(new_events[-5:]):
                            with event_feed:
                                ui.label(f'• {event}').classes('text-sm text-gray-300')
                        last_events = new_events

                    if state.get('winner'):
                        game_status.set_text(f'🏆 {state["winner"]} WINS!')
                        game_status.classes('text-yellow-400 flash')
                        start_btn.set_visibility(True)
                        stop_btn.set_visibility(False)
                    elif state.get('running'):
                        game_status.set_text('🎮 GAME IN PROGRESS')
                        game_status.classes('text-green-400', remove='text-yellow-400 flash')
                    else:
                        game_status.set_text('Ready')
                        game_status.classes('text-gray-400', remove='text-green-400 text-yellow-400 flash')

                except Exception as e:
                    print(f"UI update error: {e}")

                await asyncio.sleep(0.1)

    ui.timer(0.1, update_ui_loop, once=False)


# ============================================================================
# ENTRYPOINT
# ============================================================================

if __name__ in {"__main__", "__mp_main__"}:
    async def start_game_loop():
        global _game_loop_task
        if _game_loop_task is None:
            _game_loop_task = asyncio.create_task(game_loop())

    app.on_startup(start_game_loop)

    ui.run(title='BattlePoint - TF2 Game', port=8080, reload=False)