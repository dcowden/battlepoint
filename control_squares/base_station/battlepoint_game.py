from battlepoint_core import GameOptions, EventManager, Clock, LedMeter, Proximity,Team, ControlPoint,TeamColor, GameMode,get_team_color
# battlepoint_game.py
from typing import Optional
from enum import Enum
import asyncio
import os
import json
import re
import random
import pygame
import aiohttp
import sys
import threading
import traceback
from nicegui import ui, app
from battlepoint_core import (
    Team,
    TeamColor,
    get_team_color,
    GameMode,
    ControlPoint,
    GameOptions,
    EventManager,
    LedMeter,
    Clock,
    RealClock,
    team_text
)
from ble_scanner import EnhancedBLEScanner

class BaseGame:
    NOT_STARTED = -100

    def __init__(self):
        self.control_point: ControlPoint | None = None
        self.options: GameOptions | None = None
        self.events: EventManager | None = None
        self.owner_meter: LedMeter | None = None
        self.capture_meter: LedMeter | None = None
        self.timer1: LedMeter | None = None
        self.timer2: LedMeter | None = None
        self.clock: Clock | None = None

        self._winner: Team = Team.NOBODY
        self._red_accum_ms: int = 0
        self._blu_accum_ms: int = 0
        self._last_update_ms: int = 0
        self._start_time_ms: int = self.NOT_STARTED

    # ------------------------------------------------------------------
    def init(
        self,
        control_point: ControlPoint,
        options: GameOptions,
        event_manager: EventManager,
        owner_meter: LedMeter,
        capture_meter: LedMeter,
        timer1: LedMeter,
        timer2: LedMeter,
        clock: Clock,
    ):
        self.control_point = control_point
        self.options = options
        self.events = event_manager
        self.owner_meter = owner_meter
        self.capture_meter = capture_meter
        self.timer1 = timer1
        self.timer2 = timer2
        self.clock = clock
        self.reset_game()

    def reset_game(self):
        self._winner = Team.NOBODY
        self._red_accum_ms = 0
        self._blu_accum_ms = 0
        now = self.clock.milliseconds()
        self._last_update_ms = now
        self._start_time_ms = self.NOT_STARTED

        # match C++: both timers max = time limit
        self.timer1.setMaxValue(self.options.time_limit_seconds)
        self.timer2.setMaxValue(self.options.time_limit_seconds)

        self.control_point.init(self.options.capture_seconds)
        self.game_type_init()
        self.update_display()

    def start(self):
        # C++: start() calls resetGame() and then sets start time
        self.reset_game()
        self._start_time_ms = self.clock.milliseconds()
        if self.events:
            self.events.game_started()

    def end(self):
        # C++: end() marks cancelled; no winner
        self._winner = Team.NOBODY
        self._start_time_ms = self.NOT_STARTED
        if self.events:
            self.events.cancelled()

    # ------------------------------------------------------------------
    def is_running(self) -> bool:
        return self._start_time_ms != self.NOT_STARTED

    def is_over(self) -> bool:
        return not self.is_running() or self._winner != Team.NOBODY

    def get_seconds_elapsed(self) -> int:
        if self._start_time_ms == self.NOT_STARTED:
            return 0
        return self.clock.seconds_since(self._start_time_ms)

    def get_accumulated_seconds(self, team: Team) -> int:
        if team == Team.RED:
            return self._red_accum_ms // 1000
        elif team == Team.BLU:
            return self._blu_accum_ms // 1000
        return 0

    def get_remaining_seconds_for_team(self, team: Team) -> int:
        rem = self.options.time_limit_seconds - self.get_accumulated_seconds(team)
        return max(0, rem)

    def get_game_type_remaining_seconds(self) -> int:
        # default: global countdown
        elapsed = self.get_seconds_elapsed()
        rem = self.options.time_limit_seconds - elapsed
        return rem if rem > 0 else 0

    def get_remaining_seconds(self) -> int:
        if self._winner != Team.NOBODY:
            return 0
        return self.get_game_type_remaining_seconds()

    def get_winner(self) -> Team:
        return self._winner

    # ------------------------------------------------------------------
    def update(self):
        """Mirror C++ Game::update() semantics."""
        if not self.is_running():
            return

        # In C++ Game::update() calls controlPoint->update() here;
        # our GameBackend already did that before calling this.

        self._update_accumulated_time()

        # 1) Victory check
        winner = self.check_victory()
        if winner != Team.NOBODY:
            self._end_game_with_winner(winner)
            self.update_display()
            return

        # 2) Overtime check
        if self.check_overtime() and self.events:
            self.events.overtime()

        # 3) Ends-in-seconds announcement
        if self.events:
            self.events.ends_in_seconds(self.get_remaining_seconds())

        # 4) Update meters
        self.update_display()

    # ------------------------------------------------------------------
    def _update_accumulated_time(self):
        now = self.clock.milliseconds()
        delta = now - self._last_update_ms
        if delta < 0:
            delta = 0

        owner = self.control_point.get_owner()
        if owner == Team.RED:
            self._red_accum_ms += delta
        elif owner == Team.BLU:
            self._blu_accum_ms += delta

        self._last_update_ms = now

    def _end_game_with_winner(self, team: Team):
        self._winner = team
        self._start_time_ms = self.NOT_STARTED
        if self.events:
            self.events.victory(team)

    # ------------------------------------------------------------------
    # hooks to be overridden by game modes
    # ------------------------------------------------------------------
    def game_type_init(self):
        pass

    def update_display(self):
        pass

    def check_victory(self) -> Team:
        return Team.NOBODY

    def check_overtime(self) -> bool:
        return False


# ======================================================================
# KOTH
# ======================================================================

class KothGame(BaseGame):
    def game_type_init(self):
        # Start neutral
        self.control_point.set_owner(Team.NOBODY)
        self.timer1.fgColor(TeamColor.BLUE)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.RED)
        self.timer2.bgColor(TeamColor.BLACK)

    def update_display(self):
        # timer1 = BLU remaining; timer2 = RED remaining
        self.timer1.setValue(self.get_remaining_seconds_for_team(Team.BLU))
        self.timer2.setValue(self.get_remaining_seconds_for_team(Team.RED))

        self.capture_meter.setValue(
            int(self.control_point.get_capture_progress_percent())
        )

        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        self.owner_meter.bgColor(TeamColor.BLACK)
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()

    def get_game_type_remaining_seconds(self) -> int:
        # Remaining time is the time left for the team that owns the point
        owner = self.control_point.get_owner()
        if owner == Team.RED:
            return self.get_remaining_seconds_for_team(Team.RED)
        elif owner == Team.BLU:
            return self.get_remaining_seconds_for_team(Team.BLU)
        else:
            # No owner: effectively "hasn't started ticking down" for either side
            return self.options.time_limit_seconds

    def check_victory(self) -> Team:
        """
        You only win if:
          - Your personal timer is at or below 0
          - You own the point
          - AND there is NO enemy presence on the point
            (enemy capturing or point contested both block victory)
        """
        owner = self.control_point.get_owner()
        capturing = self.control_point.get_capturing()
        contested = getattr(self.control_point, "is_contested", lambda: False)()

        blu_remaining = self.get_remaining_seconds_for_team(Team.BLU)
        red_remaining = self.get_remaining_seconds_for_team(Team.RED)

        # BLU win conditions
        if (
            owner == Team.BLU
            and blu_remaining <= 0
            and not contested
            and capturing != Team.RED  # no RED actively taking it
        ):
            return Team.BLU

        # RED win conditions
        if (
            owner == Team.RED
            and red_remaining <= 0
            and not contested
            and capturing != Team.BLU  # no BLU actively taking it
        ):
            return Team.RED

        return Team.NOBODY

    def check_overtime(self) -> bool:
        """
        Overtime when:
          - A team's timer has run out
          - BUT that team has not secured the point, because the enemy is on it
            (enemy capturing or contested).

        i.e. "you can't win if the enemy is on the control point" ⇒ that's overtime.
        """
        owner = self.control_point.get_owner()
        capturing = self.control_point.get_capturing()
        contested = getattr(self.control_point, "is_contested", lambda: False)()

        blu_remaining = self.get_remaining_seconds_for_team(Team.BLU)
        red_remaining = self.get_remaining_seconds_for_team(Team.RED)

        # Helper: is enemy present on point in a way that blocks a clean win?
        def enemy_on_point_for(team: Team) -> bool:
            enemy = Team.RED if team == Team.BLU else Team.BLU
            return capturing == enemy or contested

        # BLU's clock out but enemy presence blocks win -> overtime
        if blu_remaining <= 0 and enemy_on_point_for(Team.BLU):
            return True

        # RED's clock out but enemy presence blocks win -> overtime
        if red_remaining <= 0 and enemy_on_point_for(Team.RED):
            return True

        return False


# ======================================================================
# GAME FACTORY
# ======================================================================

def create_game(mode: GameMode) -> BaseGame:
    if mode == GameMode.KOTH:
        return KothGame()
    else:
        raise NotImplementedError("Dont know how to start other types of games. see multimode app")



# ========================================================================
# SETTINGS PERSISTENCE
# ========================================================================

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
                'brightness': brightness,
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
                start_delay_seconds=data.get('start_delay_seconds', 5),
            )
            volume = data.get('volume', 10)
            brightness = data.get('brightness', 50)
            return options, volume, brightness
        except Exception as e:
            print(f"Error loading settings: {e}")
            return None


# ========================================================================
# SOUND SYSTEM
# ========================================================================


SOUND_MAP = {
    1: "0001_announcer_alert.mp3",
    2: "0002_announcer_alert_center_control_being_contested.mp3",
    3: "0014_announcer_last_flag.mp3",
    4: "0015_announcer_last_flag2.mp3",
    5: "0016_announcer_overtime.mp3",
    6: "0017_announcer_overtime2.mp3",
    7: "0018_announcer_sd_monkeynaut_end_crash02.mp3",
    8: "0019_announcer_stalemate.mp3",
    9: "0020_announcer_success.mp3",
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

        self.sound_map = SOUND_MAP

        self._menu_tracks = list(range(18, 26))

    def _load_path(self, sound_id: int) -> Optional[str]:
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
        """
        Play a sound.

        - Normal sounds: play immediately (interrupt current music).
        - Menu tracks (self._menu_tracks): lowest priority
          - If something is playing, queue them.
          - If nothing is playing, start them in a loop.
        """
        if not self.enabled or not self.ok:
            return

        path = self._load_path(sound_id)
        if not path:
            return

        try:
            if sound_id in self._menu_tracks:
                # Menu music: never interrupt, always lowest priority.
                if pygame.mixer.music.get_busy():
                    pygame.mixer.music.queue(path)
                    print(f"[SOUND] Queued menu track {sound_id}: {path}")
                else:
                    pygame.mixer.music.load(path)
                    pygame.mixer.music.play(loops=-1)
                    print(f"[SOUND] Looping menu track {sound_id}: {path}")
            else:
                # High-priority / normal SFX or VO: play immediately.
                pygame.mixer.music.load(path)
                pygame.mixer.music.play()
                print(f"[SOUND] Playing {sound_id}: {path}")
        except Exception as e:
            print(f"[SOUND] ERROR playing {path}: {e}")



    def queue(self, sound_id: int):
        """Queue a track to play after the current one finishes.

        - If something is already playing via mixer.music, we queue.
        - If nothing is playing, we just play it immediately (fallback).
        """
        if not self.enabled or not self.ok:
            return

        path = self._load_path(sound_id)
        if not path:
            return

        try:
            if pygame.mixer.music.get_busy():
                pygame.mixer.music.queue(path)
                print(f"[SOUND] Queued {sound_id}: {path}")
            else:
                # Nothing playing: just start it now.
                pygame.mixer.music.load(path)
                pygame.mixer.music.play()
                print(f"[SOUND] Queue-fallback playing {sound_id}: {path}")
        except Exception as e:
            print(f"[SOUND] ERROR queueing {sound_id} ({path}): {e}")


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
        """Request a menu/background track.

        Always treated as lowest priority:
        - Queued if something is already playing.
        - Loops if nothing is playing.
        """
        if not self.enabled or not self.ok:
            return
        snd_id = random.choice(self._menu_tracks)
        self.play(snd_id)


    def set_volume(self, volume: int):
        self.volume = max(0, min(30, volume))
        if getattr(self, "ok", False):
            pygame.mixer.music.set_volume(self.volume / 30.0)

    def stop(self):
        if getattr(self, "ok", False):
            pygame.mixer.music.stop()

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
        self.event_manager = EventManager(self.clock)
        self.proximity = Proximity(self.game_options, self.clock)
        self.control_point = ControlPoint(self.event_manager, self.clock)
        self.scanner = EnhancedBLEScanner(self.clock)

        self.owner_meter = LedMeter('owner', 20)
        self.capture_meter = LedMeter('capture', 20)
        self.timer1 = LedMeter('timer1', 20)
        self.timer2 = LedMeter('timer2', 20)

        # idle = dark
        for m in (self.owner_meter, self.capture_meter, self.timer1, self.timer2):
            m.fgColor(TeamColor.BLACK)
            m.bgColor(TeamColor.BLACK)
            m.setMaxValue(1)
            m.setToMin()

        self.game: Optional[BaseGame] = None

        self._phase = GamePhase.IDLE
        self._running = False
        self._countdown_total = 0
        self._countdown_started_ms = 0
        self._last_announced_second = None

        self.game_id = 0

    def configure(self, options: GameOptions):
        options.validate()
        self.game_options = options

    def start_game(self):
        self.game_id  += 1
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
        print("Starting Game: options=", self.game_options)

        self._ensure_ble(True)

        self._countdown_total = max(0, int(self.game_options.start_delay_seconds))
        self._countdown_started_ms = self.clock.milliseconds()
        self._last_announced_second = None

        for m in (self.timer1, self.timer2, self.owner_meter, self.capture_meter):
            m.setMaxValue(max(1, self._countdown_total or 1))
            m.setToMax()
            m.fgColor(TeamColor.YELLOW)
            m.bgColor(TeamColor.BLACK)

        self.event_manager.starting_game()

        if self._countdown_total > 0:
            self._phase = GamePhase.COUNTDOWN
            self._running = False
        else:
            self._do_start_game_now()

    def _ensure_ble(self, scanning: bool):
        async def _go():
            if scanning:
                await self.scanner.start_scanning()
            else:
                await self.scanner.stop_scanning()

        try:
            loop = asyncio.get_running_loop()
            loop.create_task(_go())
        except RuntimeError:
            threading.Thread(target=lambda: asyncio.run(_go()), daemon=True).start()

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

        self._ensure_ble(False)

        # meters → idle
        for m in (self.owner_meter, self.capture_meter, self.timer1, self.timer2):
            m.fgColor(TeamColor.BLACK)
            m.bgColor(TeamColor.BLACK)
            m.setMaxValue(1)
            m.setToMin()

        self.game = None

    # --- proximity hook; EnhancedGameBackend overrides this ---
    def _update_proximity(self):
        pass


    def on_game_ended(self):
        """Hook for subclasses when a game ends naturally (victory)."""
        pass

    def update(self):
        now_ms = self.clock.milliseconds()

        # COUNTDOWN
        if self._phase == GamePhase.COUNTDOWN:
            elapsed_ms = now_ms - self._countdown_started_ms
            if elapsed_ms < 0:
                elapsed_ms = 0
                self._countdown_started_ms = now_ms

            elapsed_s = elapsed_ms // 1000
            remaining = max(0, self._countdown_total - elapsed_s)

            maxv = max(1, self._countdown_total or 1)
            for m in (self.timer1, self.timer2, self.owner_meter, self.capture_meter):
                m.setMaxValue(maxv)
                m.setValue(remaining)

            self.event_manager.starts_in_seconds(remaining)

            if remaining <= 0:
                self._do_start_game_now()
            return

        # If not running, do nothing. NO proximity, NO control point updates.
        if self._phase != GamePhase.RUNNING or not self.game:
            return

        # PROXIMITY (only while running)
        try:
            self._update_proximity()
        except Exception:
            traceback.print_exc()

        # CONTROL POINT (only while running)
        self.control_point.update(self.proximity)

        # GAME LOGIC
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
                elif owner == Team.BLU:
                    self.capture_meter.fgColor(TeamColor.BLUE)
                else:
                    self.capture_meter.fgColor('#555555')
                self.capture_meter.bgColor(TeamColor.BLACK)

        if self.game.is_over():
            self._phase = GamePhase.ENDED
            self._running = False
            # optionally: stop BLE, restart menu music here if you want true "hard stop"
            self._ensure_ble(False)
            self.on_game_ended()

    def get_state(self) -> dict:
        meters = {
            'timer1': self.timer1.to_dict(),
            'timer2': self.timer2.to_dict(),
            'owner': self.owner_meter.to_dict(),
            'capture': self.capture_meter.to_dict(),
        }

        events = [ev.to_display() for ev in self.event_manager.get_events(100)]

        game = self.game
        cp = self.control_point
        prox = self.proximity
        capture_multiplier = 0
        if cp and prox:
            capturing = cp.get_capturing()
            contested = cp.is_contested()
            if not contested:
                if capturing == Team.RED:
                    capture_multiplier = max(1, prox.get_red_count())
                elif capturing == Team.BLU:
                    capture_multiplier = max(1, prox.get_blu_count())

        #print(f"Capture Multiplier: {capture_multiplier}")

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
                "game_id": self.game_id
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
                    "capture_multiplier": capture_multiplier
                },
                'red_accumulated': self.game.get_accumulated_seconds(Team.RED),
                'blu_accumulated': self.game.get_accumulated_seconds(Team.BLU),
                'meters': meters,
                'events': events,
                'game_id': self.game_id,
            }

        # IDLE
        return {
            'running': False,
            'phase': 'idle',
            'meters': meters,
            'events': events,
            'game_id': self.game_id,
        }



# ========================================================================
# ENHANCED GAME BACKEND WITH MANUAL CONTROL TOGGLE
# ========================================================================
# this is actually KothBackend ( evolutionary reasons )
class EnhancedGameBackend(GameBackend):
    """GameBackend + sound + settings + BLE + manual/ble proximity switch"""

    def __init__(self,sound_system=None):
        super().__init__()

        # sound
        self.sound_system = None
        self._menu_music_on = False

        # re-bind event manager with sound, and control point
        if sound_system is None:
            self.sound_system = SoundSystem()
        else:
            self.sound_system = sound_system
        self.event_manager = EventManager(self.clock, self.sound_system)
        self.control_point = ControlPoint(self.event_manager, self.clock)

        # settings
        self.settings_manager = SettingsManager()
        saved = self.settings_manager.load_settings()
        if saved:
            options, volume, _ = saved
            self.configure(options)
            self.sound_system.set_volume(volume)

        # menu music
        self.sound_system.play_menu_track()
        self._menu_music_on = True

        # manual control
        self.manual_control: bool = False  # False: BLE drives proximity
        self.manual_red_on: bool = False
        self.manual_blu_on: bool = False



    def on_game_ended(self):
        # Natural end: winner decided, phase already set to ENDED.
        # Restart menu music if it's not already playing.
        if not self._menu_music_on:
            self.sound_system.play_menu_track()
            self._menu_music_on = True

    # ----- manual control API -----

    def set_manual_control(self, enabled: bool):
        self.manual_control = bool(enabled)
        if not self.manual_control:
            self.manual_red_on = False
            self.manual_blu_on = False
        print(f"[MANUAL] manual_control={self.manual_control}")

    def set_manual_state(self, red: Optional[bool] = None, blu: Optional[bool] = None):
        print(f"Manual inputs: {red}, {blu}");
        if red is not None:
            self.manual_red_on = bool(red)
        if blu is not None:
            self.manual_blu_on = bool(blu)
        print(f"[MANUAL] state red={self.manual_red_on} blu={self.manual_blu_on}")

    def get_manual_state(self) -> dict:
        return {
            "manual_control": self.manual_control,
            "red": self.manual_red_on,
            "blu": self.manual_blu_on,
        }

    # ----- override proximity update -----

    def _update_proximity(self):

        counts = self.scanner.get_player_counts()

        red_on = counts.get('red', 0)
        blue_on = counts.get('blu', 0)
        mag_on = counts.get('mag', 0)
        current_owner = self.control_point.get_owner()

        # adjust for manual control
        if self.manual_control:
            # Manual mode: allow operator to simulate 1 user on
            if self.manual_red_on:
                red_on += 1
            if self.manual_blu_on:
                blue_on += 1

        # we dont know the color of a player with magnetic presence.
        # BUT we can assume that if the current owner is one team,
        # a magnetic player is the OTHER team. why would a team stand on a point it owns?
        if red_on == 0 and blue_on == 0:
            if current_owner == Team.RED:
                blue_on = mag_on
            elif current_owner == Team.BLU:
                red_on = mag_on


        if hasattr(self.proximity, 'update_counts'):
            self.proximity.update_counts(red_on, blue_on)
        else:
            self.proximity.update(red_on > 0, blue_on > 0)


    # ----- lifecycle -----

    def start_game(self):
        self.game_id += 1
        if self._menu_music_on:
            self.sound_system.stop()
            self._menu_music_on = False
        super().start_game()

    def _do_start_game_now(self):
        if self._menu_music_on:
            self.sound_system.stop()
            self._menu_music_on = False
        super()._do_start_game_now()

    def stop_game(self):
        super().stop_game()
        if not self._menu_music_on:
            self.sound_system.play_menu_track()
            self._menu_music_on = True

    # ----- BLE passthroughs -----

    async def start_bluetooth_scanning(self):
        await self.scanner.start_scanning()

    async def stop_bluetooth_scanning(self):
        await self.scanner.stop_scanning()

    def get_ble_devices_summary(self) -> dict:
        return self.scanner.get_devices_summary()

    # ----- settings -----

    def save_settings(self) -> bool:
        volume = getattr(self.sound_system, "volume", 10)
        return self.settings_manager.save_settings(self.game_options, volume=volume, brightness=50)

    def load_settings(self):
        return self.settings_manager.load_settings()
