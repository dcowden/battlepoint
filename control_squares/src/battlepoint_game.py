"""
BattlePoint - Real-life Team Fortress 2 Game
Python/NiceGUI Implementation
"""

from enum import Enum
from dataclasses import dataclass
from typing import Optional, List, Callable
import time
import asyncio
import os
import json
import random
import pygame
import aiohttp
from nicegui import ui, app


# ========================================================================
# LED METER (Python port of the C LedMeter)
# ========================================================================

class LedMeter:
    """
    Python version of the C LedMeter.
    No actual LEDs here – instead we expose a renderable state for the UI.
    """
    def __init__(self, name: str, led_count: int):
        self.name = name
        self._value = 0
        self._max_value = max(1, led_count)  # avoid div-by-zero
        self._led_count = led_count
        self._fg_color = '#000000'
        self._bg_color = '#000000'
        self._reversed = False

    def init(self):
        pass

    def reverse(self):
        self._reversed = not self._reversed

    def setValue(self, value: int):
        self._value = max(0, min(int(value), self._max_value))

    def setToMax(self):
        self.setValue(self._max_value)

    def setToMin(self):
        self.setValue(0)

    def setMaxValue(self, value: int):
        self._max_value = max(1, int(value))
        if self._value > self._max_value:
            self._value = self._max_value

    def fgColor(self, color_hex: str):
        self._fg_color = color_hex

    def bgColor(self, color_hex: str):
        self._bg_color = color_hex

    def getValue(self) -> int:
        return self._value

    def getMaxValue(self) -> int:
        return self._max_value

    def to_dict(self) -> dict:
        if self._max_value <= 0:
            pct = 0.0
        else:
            pct = (self._value / self._max_value) * 100.0
        return {
            'name': self.name,
            'value': self._value,
            'max': self._max_value,
            'percent': pct,
            'fg': self._fg_color,
            'bg': self._bg_color,
            'reversed': self._reversed,
        }


# ============================================================================
# CONSTANTS AND ENUMS
# ============================================================================

class Team(Enum):
    RED = 1
    BLU = 2
    NOBODY = 3
    BOTH = 4


class GameMode(Enum):
    KOTH = 0  # King of the Hill
    AD = 1    # Attack/Defend
    CP = 2    # Control Point


class TeamColor:
    RED = '#FF0000'
    BLUE = '#0000FF'
    BLACK = '#000000'
    AQUA = '#00FFFF'
    PURPLE = '#800080'
    YELLOW = '#FFFF00'


# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

def get_team_color(team: Team) -> str:
    if team == Team.NOBODY:
        return TeamColor.BLACK
    elif team == Team.BLU:
        return TeamColor.BLUE
    elif team == Team.RED:
        return TeamColor.RED
    else:
        return TeamColor.AQUA


def team_text(team: Team) -> str:
    if team == Team.RED:
        return "RED"
    elif team == Team.BLU:
        return "BLU"
    elif team == Team.BOTH:
        return "R&B"
    elif team == Team.NOBODY:
        return "---"
    return "???"


def team_text_char(team: Team) -> str:
    if team == Team.RED:
        return 'R'
    elif team == Team.BLU:
        return 'B'
    elif team == Team.BOTH:
        return '+'
    elif team == Team.NOBODY:
        return '-'
    return '?'


def game_mode_text(mode: GameMode) -> str:
    if mode == GameMode.KOTH:
        return "KOTH"
    elif mode == GameMode.AD:
        return "AD"
    elif mode == GameMode.CP:
        return "CP"
    return "??"


# ============================================================================
# CLOCK ABSTRACTION
# ============================================================================

class Clock:
    def milliseconds(self) -> int:
        raise NotImplementedError

    def seconds_since(self, start_millis: int) -> int:
        return (self.milliseconds() - start_millis) // 1000


class RealClock(Clock):
    def milliseconds(self) -> int:
        return int(time.time() * 1000)


class TestClock(Clock):
    def __init__(self):
        self._time = 0

    def milliseconds(self) -> int:
        return self._time

    def set_time(self, millis: int):
        self._time = millis

    def add_millis(self, millis: int):
        self._time += millis

    def add_seconds(self, seconds: int):
        self._time += seconds * 1000


# ============================================================================
# BLUETOOTH TAG IMPLEMENTATION (Expanded)
# ============================================================================

class TagType(Enum):
    PLAYER = 1
    CONTROL_SQUARE = 2
    HEALTH_SQUARE = 3


@dataclass
class BluetoothTag:
    id: int
    tag_type: TagType
    team: Team = Team.NOBODY
    last_seen: int = 0
    rssi: int = 0

    def is_active(self, current_time: int, timeout_ms: int = 1000) -> bool:
        return (current_time - self.last_seen) < timeout_ms


class BluetoothScanner:
    def __init__(self, clock: Clock):
        self.clock = clock
        self.tags: dict[int, BluetoothTag] = {}
        self.service_uuid = "YOUR_SERVICE_UUID_HERE"
        self._scanning = False

    async def start_scanning(self):
        self._scanning = True

    async def stop_scanning(self):
        self._scanning = False

    def parse_tag_data(self, raw_data: bytes) -> Optional[BluetoothTag]:
        if len(raw_data) < 4:
            return None
        try:
            tag_type = TagType(raw_data[0])
            tag_id = (raw_data[1] << 8) | raw_data[2]
            team = Team(raw_data[3]) if raw_data[3] in [1, 2] else Team.NOBODY
            return BluetoothTag(
                id=tag_id,
                tag_type=tag_type,
                team=team,
                last_seen=self.clock.milliseconds()
            )
        except (ValueError, IndexError):
            return None

    def get_active_tags(self, tag_type: Optional[TagType] = None) -> List[BluetoothTag]:
        current_time = self.clock.milliseconds()
        active = [tag for tag in self.tags.values() if tag.is_active(current_time)]
        if tag_type:
            active = [tag for tag in active if tag.tag_type == tag_type]
        return active

    def get_players_near_control_point(self, control_point_id: int) -> dict[Team, int]:
        counts = {Team.RED: 0, Team.BLU: 0}
        for tag in self.get_active_tags(TagType.PLAYER):
            if tag.team in [Team.RED, Team.BLU]:
                counts[tag.team] += 1
        return counts


# ============================================================================
# COOLDOWN TIMER
# ============================================================================

class CooldownTimer:
    def __init__(self, interval_ms: int, clock: Clock):
        self.interval_ms = interval_ms
        self.clock = clock
        self.last_event = -interval_ms

    def can_run(self) -> bool:
        current = self.clock.milliseconds()
        if (current - self.last_event) >= self.interval_ms:
            self.last_event = current
            return True
        return False

    def is_in_cooldown(self) -> bool:
        return not self.can_run()


# ============================================================================
# GAME OPTIONS
# ============================================================================

@dataclass
class GameOptions:
    mode: GameMode = GameMode.KOTH
    capture_seconds: int = 20
    capture_button_threshold_seconds: int = 5
    time_limit_seconds: int = 60
    start_delay_seconds: int = 5

    def time_limit_millis(self) -> int:
        return self.time_limit_seconds * 1000

    def validate(self):
        if self.capture_seconds >= self.time_limit_seconds:
            self.capture_seconds = self.time_limit_seconds - 1
        if self.capture_button_threshold_seconds >= self.capture_seconds:
            self.capture_button_threshold_seconds = self.capture_seconds - 1


# ============================================================================
# SIMPLE METER
# ============================================================================

class SimpleMeter:
    def __init__(self):
        self._value = 0
        self._max_value = 100

    def get_value(self) -> int:
        return self._value

    def get_max_value(self) -> int:
        return self._max_value

    def set_value(self, value: int):
        self._value = max(0, min(value, self._max_value))

    def set_max_value(self, max_value: int):
        self._max_value = max_value

    def set_to_max(self):
        self._value = self._max_value

    def set_to_min(self):
        self._value = 0


# ============================================================================
# PROXIMITY
# ============================================================================

class Proximity:
    def __init__(self, game_options: GameOptions, clock: Clock):
        self.game_options = game_options
        self.clock = clock
        self.last_red_press = -1
        self.last_blu_press = -1

    def red_button_press(self):
        self.last_red_press = self.clock.milliseconds()

    def blu_button_press(self):
        self.last_blu_press = self.clock.milliseconds()

    def update(self, red_close: bool, blu_close: bool):
        if red_close:
            self.red_button_press()
        if blu_close:
            self.blu_button_press()

    def is_red_close(self) -> bool:
        if self.last_red_press < 0:
            return False
        threshold_ms = self.game_options.capture_button_threshold_seconds * 1000
        return (self.clock.milliseconds() - self.last_red_press) < threshold_ms

    def is_blu_close(self) -> bool:
        if self.last_blu_press < 0:
            return False
        threshold_ms = self.game_options.capture_button_threshold_seconds * 1000
        return (self.clock.milliseconds() - self.last_blu_press) < threshold_ms

    def is_team_close(self, team: Team) -> bool:
        if team == Team.RED:
            return self.is_red_close()
        elif team == Team.BLU:
            return self.is_blu_close()
        return False


# ============================================================================
# EVENT MANAGER
# ============================================================================

class EventManager:
    def __init__(self, clock: Clock, sound_system=None):
        self.clock = clock
        self.capture_timer = CooldownTimer(20000, clock)
        self.contest_timer = CooldownTimer(20000, clock)
        self.overtime_timer = CooldownTimer(5000, clock)
        self.start_time_timer = CooldownTimer(900, clock)
        self.end_time_timer = CooldownTimer(900, clock)
        self.cp_alert_interval_ms = 5000
        self.events: list[str] = []
        self.sound_system = sound_system

    def init(self, cp_alert_interval_seconds: int):
        self.cp_alert_interval_ms = cp_alert_interval_seconds * 1000

    def _add_event(self, event: str):
        self.events.append(event)
        print(f"[EVENT] {event}")

    def _play(self, snd_id: int):
        if self.sound_system:
            self.sound_system.play(snd_id)

    # === match C ===
    def control_point_being_captured(self, team: Team):
        if self.capture_timer.can_run():
            self._add_event(f"Control Point Being Captured by {team_text(team)}")
            # C: SND_SOUNDS_0014_ANNOUNCER_LAST_FLAG
            self._play(3)

    def control_point_contested(self):
        if self.contest_timer.can_run():
            self._add_event("Control Point is Contested!")
            # C: SND_SOUNDS_0002_ANNOUNCER_ALERT_CENTER_CONTROL_BEING_CONTESTED
            self._play(2)

    def control_point_captured(self, team: Team):
        self._add_event(f"Control Point Captured by {team_text(team)}")
        # C: SND_SOUNDS_0025_ANNOUNCER_WE_CAPTURED_CONTROL
        self._play(14)

    def starting_game(self):
        self._add_event("Starting Game")
        # C: _player->play(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED);
        # our map: 10 -> "0021_announcer_time_added.mp3"
        self._play(10)

    def game_started(self):
        self._add_event("Game Started")
        # C: SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4
        self._play(11)

    def victory(self, team: Team):
        self._add_event(f"Victory for {team_text(team)}!")
        # C: SND_SOUNDS_0023_ANNOUNCER_VICTORY
        self._play(12)

    def overtime(self):
        if self.overtime_timer.can_run():
            self._add_event("OVERTIME!")
            # C: SND_SOUNDS_0017_ANNOUNCER_OVERTIME2
            self._play(6)

    def cancelled(self):
        self._add_event("Game Cancelled")
        # C: _player->play(SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10);
        self._play(17)

    def starts_in_seconds(self, secs: int):
        if self.start_time_timer.can_run():
            if secs in [30, 20, 10, 5, 4, 3, 2, 1, 60]:
                self._add_event(f"Game starts in {secs} seconds")
                starts_map = {
                    60: 34,
                    30: 30,
                    20: 28,
                    10: 26,
                    5: 33,
                    4: 32,
                    3: 31,
                    2: 29,
                    1: 27,
                }
                sid = starts_map.get(secs)
                if sid:
                    self._play(sid)

    def ends_in_seconds(self, secs: int):
        if self.end_time_timer.can_run():
            if secs in [120, 60, 30, 20, 10, 5, 4, 3, 2, 1]:
                self._add_event(f"Game ends in {secs} seconds")
                ends_map = {
                    120: 38,
                    60: 44,
                    30: 40,
                    20: 37,
                    10: 35,
                    5: 43,
                    4: 42,
                    3: 41,
                    2: 39,
                    1: 36,
                }
                sid = ends_map.get(secs)
                if sid:
                    self._play(sid)



# ============================================================================
# CONTROL POINT
# ============================================================================

class ControlPoint:
    def __init__(self, event_manager: EventManager, clock: Clock):
        self.event_manager = event_manager
        self.clock = clock
        self._owner = Team.NOBODY
        self._on = Team.NOBODY
        self._capturing = Team.NOBODY
        self._value = 0
        self._contested = False
        self._seconds_to_capture = 20
        self._last_update_time = 0
        self._enable_red_capture = True
        self._enable_blu_capture = True
        self._should_contest_message = True

    def init(self, seconds_to_capture: int):
        self._seconds_to_capture = seconds_to_capture
        self._capturing = Team.NOBODY
        self._on = Team.NOBODY
        self._owner = Team.NOBODY
        self._value = 0
        self._contested = False
        self._last_update_time = self.clock.milliseconds()
        self._enable_red_capture = True
        self._enable_blu_capture = True
        self._should_contest_message = True

    def set_red_capture(self, enabled: bool):
        self._enable_red_capture = enabled

    def set_blu_capture(self, enabled: bool):
        self._enable_blu_capture = enabled

    def get_owner(self) -> Team:
        return self._owner

    def get_on(self) -> Team:
        return self._on

    def get_capturing(self) -> Team:
        return self._capturing

    def get_value(self) -> int:
        return self._value

    def is_contested(self) -> bool:
        return self._contested

    def captured(self) -> bool:
        return (self._owner != Team.NOBODY and
                (self._on == Team.NOBODY or self._on == self._owner) and
                self._value <= 0)

    def captured_by(self, team: Team) -> bool:
        return self.captured() and self._owner == team

    def set_owner(self, team: Team):
        self._owner = team

    def update(self, proximity: Proximity):
        red_on = proximity.is_red_close()
        blu_on = proximity.is_blu_close()

        self._contested = False
        is_one_team_on = False

        if red_on and not blu_on:
            self._on = Team.RED
            is_one_team_on = True
        elif blu_on and not red_on:
            self._on = Team.BLU
            is_one_team_on = True
        elif blu_on and red_on:
            self._on = Team.BOTH
            self._contested = True
            if self._should_contest_message:
                self.event_manager.control_point_contested()
                self._should_contest_message = False
        else:
            self._on = Team.NOBODY

        if not self._contested:
            self._should_contest_message = True

        if self._capturing == Team.NOBODY and is_one_team_on:
            if self._owner == Team.NOBODY or self._on != self._owner:
                can_capture = ((self._on == Team.RED and self._enable_red_capture) or
                               (self._on == Team.BLU and self._enable_blu_capture))
                if can_capture:
                    self._capturing = self._on
                    self.event_manager.control_point_being_captured(self._capturing)

        millis_since_last = self.clock.milliseconds() - self._last_update_time

        if is_one_team_on:
            if self._capturing == self._on:
                self._inc_capture(millis_since_last)
            elif self._capturing != self._on:
                self._dec_capture(millis_since_last)
        elif self._on == Team.NOBODY:
            self._dec_capture(millis_since_last)

        self._check_capture()
        self._last_update_time = self.clock.milliseconds()

    def _inc_capture(self, millis: int):
        self._value += millis
        capture_ms = self._seconds_to_capture * 1000
        if self._value > capture_ms:
            self._value = capture_ms

    def _dec_capture(self, millis: int):
        self._value -= millis
        if self._value <= 0:
            self._capturing = Team.NOBODY
            self._value = 0

    def _check_capture(self):
        if self._value >= self._seconds_to_capture * 1000:
            self._owner = self._capturing
            self._capturing = Team.NOBODY
            self._value = 0
            self.event_manager.control_point_captured(self._owner)

    def get_owner_color(self) -> str:
        return get_team_color(self._owner)

    def get_capturing_color(self) -> str:
        if self._contested:
            return TeamColor.PURPLE
        return get_team_color(self._capturing)

    def get_capture_progress_percent(self) -> float:
        if self._seconds_to_capture == 0:
            return 0
        return (self._value / (self._seconds_to_capture * 1000)) * 100


# ============================================================================
# BASE GAME CLASS
# ============================================================================

# ========================================================================
# PYTHON PORT OF THE ORIGINAL C/ARDUINO GAME LOGIC
# ========================================================================

class BaseGame:
    """
    Port of the C/Arduino Game logic you pasted.
    This class depends on:
      - control_point: must have update(...), get_owner(), captured_by(team), get_capturing(), is_contested(), get_capture_progress_percent()
      - event_manager: must have game_started(), cancelled(), victory(team), overtime(), ends_in_seconds(sec)
      - options: GameOptions with mode, capture_seconds, time_limit_seconds, etc.
      - clock: must have milliseconds()
    """

    NOT_STARTED = -100  # same idea as C

    def __init__(self):
        self.control_point = None
        self.options: GameOptions | None = None
        self.event_manager: EventManager | None = None
        self.proximity: Proximity | None = None
        self.clock: Clock | None = None

        self.owner_meter: LedMeter | None = None
        self.capture_meter: LedMeter | None = None
        self.timer1: LedMeter | None = None
        self.timer2: LedMeter | None = None

        self._winner: Team = Team.NOBODY
        self._red_millis: int = 0
        self._blu_millis: int = 0
        self._last_update_ms: int = 0
        self._start_time_ms: int = self.NOT_STARTED
        self._should_announce_overtime: bool = True

    # ------------------------------------------------------------
    # init like C: wires deps, then reset
    # ------------------------------------------------------------
    def init(self,
             control_point: ControlPoint,
             options: GameOptions,
             event_manager: EventManager,
             owner_meter: LedMeter,
             capture_meter: LedMeter,
             timer1: LedMeter,
             timer2: LedMeter,
             clock: Clock,
             proximity: Proximity | None = None):
        self.control_point = control_point
        self.options = options
        self.event_manager = event_manager
        self.owner_meter = owner_meter
        self.capture_meter = capture_meter
        self.timer1 = timer1
        self.timer2 = timer2
        self.clock = clock
        self.proximity = proximity
        self.reset_game()

    # ------------------------------------------------------------
    def reset_game(self):
        self._winner = Team.NOBODY
        self._red_millis = 0
        self._blu_millis = 0
        now = self.clock.milliseconds()
        self._last_update_ms = now
        self._start_time_ms = self.NOT_STARTED

        # meters set to time limit just like C
        self.timer1.setMaxValue(self.options.time_limit_seconds)
        self.timer2.setMaxValue(self.options.time_limit_seconds)

        # subclass can set colors etc.
        self.game_type_init()

        # sync meters right away
        self.update_display()

    # ------------------------------------------------------------
    def start(self):
        # exactly like C: init meters, event mgr, control point
        self.timer1.init()
        self.timer2.init()
        self.event_manager.init(self.options.capture_button_threshold_seconds)
        self.control_point.init(self.options.capture_seconds)

        # let the actual game class (KothGame, ADGame, CPGame) own colors
        # C does this in each gameTypeInit()/start()
        self.game_type_init()

        self._winner = Team.NOBODY
        self._red_millis = 0
        self._blu_millis = 0
        self._last_update_ms = self.clock.milliseconds()
        self._start_time_ms = self.clock.milliseconds()
        self._should_announce_overtime = True

        # C reverses timer1, timer2 in Game ctor/start
        self.timer1.reverse()
        self.timer2.reverse()

        # meters show time limit
        self.timer1.setMaxValue(self.options.time_limit_seconds)
        self.timer2.setMaxValue(self.options.time_limit_seconds)

        # announce
        self.event_manager.game_started()

    # ------------------------------------------------------------
    def end(self):
        # C version: winner = NOBODY and cancelled()
        self._winner = Team.NOBODY
        self.event_manager.cancelled()
        self._start_time_ms = self.NOT_STARTED

    # ------------------------------------------------------------
    def is_over(self) -> bool:
        return self._winner != Team.NOBODY

    def is_running(self) -> bool:
        # C/Arduino: return !isOver()
        return not self.is_over()

    # ------------------------------------------------------------
    def get_accumulated_seconds(self, team: Team) -> int:
        if team == Team.RED:
            return self._red_millis // 1000
        elif team == Team.BLU:
            return self._blu_millis // 1000
        return 0

    def get_winner(self) -> Team:
        return self._winner

    def get_remaining_seconds_for_team(self, team: Team) -> int:
        # like C: (timeLimitMillis - teamMillis) / 1000
        limit_ms = self.options.time_limit_seconds * 1000
        if team == Team.RED:
            return max(0, (limit_ms - self._red_millis) // 1000)
        elif team == Team.BLU:
            return max(0, (limit_ms - self._blu_millis) // 1000)
        return 0

    def get_remaining_seconds(self) -> int:
        # C: if winner set -> 0 else gameTypeRemainingSeconds
        if self._winner != Team.NOBODY:
            return 0
        # for KOTH and AD we can define as “global” remaining, but
        # in C they call ends_in_seconds for the relevant team,
        # so here we just give the max of both to avoid UI showing 0 too early
        red_left = self.get_remaining_seconds_for_team(Team.RED)
        blu_left = self.get_remaining_seconds_for_team(Team.BLU)
        return max(red_left, blu_left)


    # ------------------------------------------------------------
    def game_type_init(self):
        """Hook for subclasses – mirrors C's gameTypeInit()."""
        pass

    # ------------------------------------------------------------
    def update_display(self):
        """What C does at the end of update(): write to meters."""
        # capture meter = CP percent
        cp_pct = self.control_point.get_capture_progress_percent()
        self.capture_meter.setMaxValue(100)
        self.capture_meter.setValue(int(cp_pct))

        # owner meter: in the C code you sometimes just show owner/max.
        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()

        # timers: show remaining for each team
        red_left = self.get_remaining_seconds_for_team(Team.RED)
        blu_left = self.get_remaining_seconds_for_team(Team.BLU)
        self.timer1.setValue(red_left)
        self.timer2.setValue(blu_left)

    # ------------------------------------------------------------
    def _end_game_with_winner(self, winner: Team):
        self._winner = winner
        self.event_manager.victory(winner)
        self._start_time_ms = self.NOT_STARTED

    # ------------------------------------------------------------
    def update(self):
        """Subclasses must implement EXACT per-mode logic (KOTH / AD / CP)."""
        raise NotImplementedError


# ============================================================================
# GAME MODES
# ============================================================================

class KothGame(BaseGame):
    def game_type_init(self):
        # C:
        # _controlPoint->setOwner(Team::NOBODY);
        # _timer1Meter->setColors(BLUE, BLACK);
        # _timer2Meter->setColors(RED,  BLACK);
        self.control_point.set_owner(Team.NOBODY)
        self.timer1.fgColor(TeamColor.BLUE)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.RED)
        self.timer2.bgColor(TeamColor.BLACK)

    def update(self):
        if self.is_over():
            return

        self.control_point.update(self.proximity)

        now = self.clock.milliseconds()
        millis_since_last = now - self._last_update_ms

        # accumulate for owner
        owner = self.control_point.get_owner()
        if owner == Team.RED:
            self._red_millis += millis_since_last
        elif owner == Team.BLU:
            self._blu_millis += millis_since_last

        # now get remaining like C
        red_left = self.get_remaining_seconds_for_team(Team.RED)
        blu_left = self.get_remaining_seconds_for_team(Team.BLU)

        # victory / OT like C
        if blu_left <= 0 and self.control_point.captured_by(Team.BLU):
            self._end_game_with_winner(Team.BLU)
            self.update_display()
            self._last_update_ms = now
            return
        if red_left <= 0 and self.control_point.captured_by(Team.RED):
            self._end_game_with_winner(Team.RED)
            self.update_display()
            self._last_update_ms = now
            return

        # overtime
        if blu_left <= 0 and self.control_point.get_capturing() == Team.RED:
            if self._should_announce_overtime:
                self.event_manager.overtime()
                self._should_announce_overtime = False
        if red_left <= 0 and self.control_point.get_capturing() == Team.BLU:
            if self._should_announce_overtime:
                self.event_manager.overtime()
                self._should_announce_overtime = False

        # ends-in-seconds only for current owner, like C
        if owner == Team.BLU:
            self.event_manager.ends_in_seconds(blu_left)
        elif owner == Team.RED:
            self.event_manager.ends_in_seconds(red_left)

        self.update_display()
        self._last_update_ms = now

    def update_display(self):
        # C:
        # timer1 = BLU remaining
        # timer2 = RED remaining
        self.timer1.setValue(self.get_remaining_seconds_for_team(Team.BLU))
        self.timer2.setValue(self.get_remaining_seconds_for_team(Team.RED))
        self.capture_meter.setMaxValue(100)
        self.capture_meter.setValue(int(self.control_point.get_capture_progress_percent()))
        # owner meter
        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()


class ADGame(BaseGame):
    def game_type_init(self):
        # C behavior
        self.control_point.set_red_capture(False)
        self.control_point.set_owner(Team.RED)
        self.timer1.fgColor(TeamColor.YELLOW)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.YELLOW)
        self.timer2.bgColor(TeamColor.BLACK)

    def start(self):
        # keep BaseGame.start() logic, but we already set colors above
        super().start()
        # ensure CP is RED at start
        self.control_point.set_owner(Team.RED)
        self.control_point.set_red_capture(False)

    def update_display(self):
        # C: both timers show SAME remaining
        remaining = self.get_remaining_seconds()
        self.timer1.setValue(remaining)
        self.timer2.setValue(remaining)
        # capture + owner same as C
        self.capture_meter.setMaxValue(100)
        self.capture_meter.setValue(int(self.control_point.get_capture_progress_percent()))
        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()



class CPGame(BaseGame):
    def update(self):
        if self.is_over():
            return

        self.control_point.update(self.proximity)

        now = self.clock.milliseconds()
        millis_since_last = now - self._last_update_ms

        # whoever owns it accumulates
        owner = self.control_point.get_owner()
        if owner == Team.RED:
            self._red_millis += millis_since_last
        elif owner == Team.BLU:
            self._blu_millis += millis_since_last

        # CP game: ends when global time is up – winner = most time
        elapsed_s = (now - self._start_time_ms) // 1000
        if elapsed_s > self.options.time_limit_seconds:
            if self._red_millis > self._blu_millis:
                self._end_game_with_winner(Team.RED)
            else:
                self._end_game_with_winner(Team.BLU)
            self.update_display()
            self._last_update_ms = now
            return

        self.update_display()
        self._last_update_ms = now


# ============================================================================
# GAME FACTORY
# ============================================================================

def create_game(mode: GameMode) -> BaseGame:
    if mode == GameMode.KOTH:
        return KothGame()
    elif mode == GameMode.AD:
        return ADGame()
    elif mode == GameMode.CP:
        return CPGame()
    return KothGame()


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
        self.event_manager = EventManager(self.clock, sound_system=None)
        self.proximity = Proximity(self.game_options, self.clock)
        self.control_point = ControlPoint(self.event_manager, self.clock)
        self.scanner = BluetoothScanner(self.clock)

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
            self.proximity,
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

        # start the actual game logic (this already does meters, cp init, etc.)
        self.game.start()

        # IMPORTANT: mirror the C flow – announce *here* at the backend level
        # so that even if a game subclass forgets to call super().start()
        # we still get the "Game Started" event + sound.
        if self.event_manager:
            self.event_manager.game_started()

        # make sure UI meters are in sync
        self.game.update_display()

        # flip to running
        self._phase = GamePhase.RUNNING
        self._running = True

        # kill menu music if any
        ss = getattr(self, "sound_system", None)
        if ss:
            ss.stop()

    def stop_game(self):
        # 1) tell current game to end (this will fire "Game Cancelled")
        if self.game:
            self.game.end()

        # 2) backend phase -> idle
        self._phase = GamePhase.IDLE
        self._running = False
        self._last_announced_second = None

        # 3) RETURN METERS TO IDLE (same as __init__)
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

        # 4) (optional) drop the reference; next start creates a fresh game
        # this also mimics "we're not in a game right now"
        self.game = None

        # 5) NOTE: we do NOT start menu music here in the base class.
        # The enhanced subclass can decide that.

    def update(self):
        now_ms = self.clock.milliseconds()

        # -------------------------------------------------
        # 1) COUNTDOWN PHASE
        # -------------------------------------------------
        if self._phase == GamePhase.COUNTDOWN:
            # how long since we started counting down
            elapsed_ms = now_ms - self._countdown_started_ms
            elapsed_s = elapsed_ms // 1000
            remaining = max(0, self._countdown_total - elapsed_s)

            # keep meters in countdown style (yellow, full → down)
            for m in (self.timer1, self.timer2, self.owner_meter, self.capture_meter):
                m.setMaxValue(max(1, self._countdown_total or 1))
                # show remaining as value so UI can show percent correctly
                m.setValue(remaining)

            # announce “Game starts in X seconds” from backend too
            if remaining != self._last_announced_second:
                self.event_manager.starts_in_seconds(remaining)
                self._last_announced_second = remaining

            # when we hit zero → actually start the game
            if remaining <= 0:
                self._do_start_game_now()

            return  # important: don't fall through

        # -------------------------------------------------
        # 2) RUNNING PHASE
        # -------------------------------------------------
        if self._phase == GamePhase.RUNNING:
            if self.game:
                self.game.update()

                # update capture meter from control point (C++ updateCaptureMeter())
                cp = self.control_point
                cp_pct = cp.get_capture_progress_percent()
                self.capture_meter.setMaxValue(100)
                self.capture_meter.setValue(int(cp_pct))

                # color: who is capturing?
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

                # if game mode decided winner
                if self.game.is_over():
                    self._phase = GamePhase.ENDED
                    self._running = False


            return

        # -------------------------------------------------
        # 3) ENDED / IDLE
        # -------------------------------------------------
        # nothing special – keep whatever meters were last set
        return

    def get_state(self) -> dict:
        meters = {
            'timer1': self.timer1.to_dict(),
            'timer2': self.timer2.to_dict(),
            'owner': self.owner_meter.to_dict(),
            'capture': self.capture_meter.to_dict(),
        }

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
                'events': self.event_manager.events[-10:],
            }

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
                'remaining_seconds': remaining_seconds,  # ← THIS is the one UI must show
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
                'events': self.event_manager.events[-10:],
            }


        return {
            'running': False,
            'phase': 'idle',
            'meters': meters,
            'events': self.event_manager.events[-10:],
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
    def __init__(self, base_dir: str = "src/sounds"):
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
        self.bluetooth_scanner = BluetoothScanner(self.clock)

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


@app.get("/api/bluetooth/tags")
async def get_bluetooth_tags():
    tags = enhanced_backend.bluetooth_scanner.get_active_tags()
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
import aiohttp  # make sure this is here


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
      .bp-main {
        height: calc(100vh - 52px - 140px);
        display: flex;
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
      }
      .bp-side {
        width: 50vw;
        min-width: 140px;
        height: 100%;
        display: flex;
        flex-direction: column;
        gap: 0.75rem;
        align-items: center;
        justify-content: center;
      }
      .bp-vert-shell {
        width: 140px;
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
        font-size: min(16rem, 20vh);
        line-height: 1;
        font-weight: 700;
        color: #fff;
        text-align: center;
      }
      .bp-status {
        font-size: 1.4rem;
        color: #ddd;
      }
      /* wide capture bar */
    .bp-capture-shell {
        width: min(60vw, 900px);
        height: 64px;              /* was 32px */
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
        height: 100%;              /* will now fill 64px */
        width: 0%;
        background: #6600ff;
        transition: width 0.12s linear;
    }
      .bp-bottom {
        height: 140px;
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
            background: #000000;  /* black, like LEDs off */
            position: relative;
            overflow: hidden;
        }
      .bp-horiz-red {
        position: absolute;
        top: 0;
        left: 0;
        height: 100%;
        width: 50%;
        background: #ff0000;
        transition: width 0.15s linear;
      }
      .bp-buttons {
        position: absolute;
        bottom: 0.4rem;
        left: 50%;
        transform: translateX(-50%);
        display: flex;
        gap: 0.5rem;
      }
    </style>
    """)

    with ui.element('div').classes('bp-root'):

        # top bar
        with ui.element('div').classes('bp-topbar'):
            ui.label('BattlePoint').classes('text-white text-lg font-bold')
            with ui.row().classes('gap-2'):
                ui.button('Game', on_click=lambda: ui.navigate.to('/')).props('flat color=white')
                ui.button('Settings', on_click=lambda: ui.navigate.to('/settings')).props('flat color=white')
                ui.button('Debug', on_click=lambda: ui.navigate.to('/debug')).props('flat color=white')

        # main
        with ui.element('div').classes('bp-main'):

            # LEFT: timer1 -> (KOTH: BLUE)
            with ui.element('div').classes('bp-side'):
                ui.label('TIMER1').classes('text-2xl font-bold text-blue-400')
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
                ui.label('TIMER2').classes('text-2xl font-bold text-red-400')
                with ui.element('div').classes('bp-vert-shell'):
                    blue_fill = ui.element('div').classes('bp-vert-fill')

        # bottom: owner meter
        with ui.element('div').classes('bp-bottom'):
            with ui.element('div').classes('bp-bottom-wrapper'):
                with ui.element('div').classes('bp-horiz-shell'):
                    horiz_red = ui.element('div').classes('bp-horiz-red')
                with ui.element('div').classes('bp-buttons'):
                    ui.button('Red', on_click=lambda: simulate_red()).props('color=red')
                    ui.button('Blue', on_click=lambda: simulate_blue()).props('color=blue')
                    ui.button('Start', on_click=lambda: start_game()).props('color=green')
                    ui.button('Stop', on_click=lambda: stop_game()).props('color=orange')

    # helpers
    async def simulate_red():
        await ui.run_javascript('fetch("/api/proximity/red", {method: "POST"})')

    async def simulate_blue():
        await ui.run_javascript('fetch("/api/proximity/blue", {method: "POST"})')

    async def start_game():
        await ui.run_javascript('fetch("/api/start", {method: "POST"})')

    async def stop_game():
        await ui.run_javascript('fetch("/api/stop", {method: "POST"})')

    # map event text -> sound id (same as before)
    def sound_id_for_event(txt: str) -> int | None:
        if txt.startswith("Control Point Being Captured by "):
            return 3
        if txt == "Control Point is Contested!":
            return 2
        if txt.startswith("Control Point Captured by "):
            return 14
        if txt == "Starting Game":
            return 10
        if txt == "Game Started":
            return 11
        if txt.startswith("Victory for "):
            return 12
        if txt == "OVERTIME!":
            return 6
        if txt == "Game Cancelled":
            return 17

        if txt.startswith("Game starts in "):
            try:
                secs = int(txt.replace("Game starts in ", "").replace(" seconds", ""))
            except ValueError:
                return None
            m = {60: 34, 30: 30, 20: 28, 10: 26, 5: 33, 4: 32, 3: 31, 2: 29, 1: 27}
            return m.get(secs)

        if txt.startswith("Game ends in "):
            try:
                secs = int(txt.replace("Game ends in ", "").replace(" seconds", ""))
            except ValueError:
                return None
            m = {120: 38, 60: 44, 30: 40, 20: 37, 10: 35, 5: 43, 4: 42, 3: 41, 2: 39, 1: 36}
            return m.get(secs)
        return None

    async def update_ui():
        session: aiohttp.ClientSession | None = None
        last_events: list[str] = []
        last_countdown: int | None = None

        # which countdown seconds should speak
        countdown_calls = {60, 30, 20, 10, 5, 4, 3, 2, 1}

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

            # TIMER1 (blue in KOTH)
            m1 = meters.get('timer1')
            if m1:
                red_fill.style(f'height: {m1.get("percent", 0)}%; background: {m1.get("fg", "#0000FF")};')

            # TIMER2 (red in KOTH)
            m2 = meters.get('timer2')
            if m2:
                blue_fill.style(f'height: {m2.get("percent", 0)}%; background: {m2.get("fg", "#FF0000")};')

            # CAPTURE BAR (wide)
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

                # if the meter is black (no owner), show neutral 50/50 red/blue
                if fg.lower() in ("#000000", "black"):
                    pct = 50
                    fg = "#ff0000"

                horiz_red.style(f'width: {pct}%; background: {fg};')

            # CLOCK / PHASE
            if phase == 'countdown':
                cd = state.get('countdown_remaining', 0)
                game_clock.set_text(f'{cd}')
                status_label.set_text('COUNTDOWN...')
                # backend already played the countdown sound via EventManager
                last_countdown = cd

            else:
                # prefer the real match clock if present
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

            # EVENTS -> SERVER SOUNDS (for non-countdown stuff)
            # EVENTS: just display them; backend already played sounds
            new_events = state.get('events', [])
            if new_events != last_events:
                # you could update a log here if you had one on this page
                last_events = new_events

            await asyncio.sleep(0.1)

    ui.timer(0.1, update_ui, once=False)


# ============================================================================
# SETTINGS PAGE
# ============================================================================

@ui.page('/settings')
def settings_ui():
    ui.colors(primary='#1976D2')

    with ui.column().classes('w-full p-8 max-w-2xl mx-auto'):
        ui.label('Game Settings').classes('text-3xl font-bold mb-4')
        ui.button('← Back to Game', on_click=lambda: ui.navigate.to('/')).classes('mb-4')

        mode_select = ui.select(
            ['KOTH', 'AD', 'CP'],
            label='Game Mode',
            value='KOTH'
        ).classes('w-full')

        time_limit = ui.number('Time Limit (seconds)', value=60, min=10, max=600).classes('w-full')
        capture_time = ui.number('Capture Time (seconds)', value=20, min=5, max=120).classes('w-full')
        button_threshold = ui.number('Button Threshold (seconds)', value=5, min=1, max=30).classes('w-full')
        start_delay = ui.number('Start Delay (seconds)', value=5, min=0, max=60).classes('w-full')

        async def save_settings():
            mode_map = {'KOTH': 0, 'AD': 1, 'CP': 2}
            options = {
                'mode': mode_map[mode_select.value],
                'time_limit_seconds': int(time_limit.value),
                'capture_seconds': int(capture_time.value),
                'capture_button_threshold_seconds': int(button_threshold.value),
                'start_delay_seconds': int(start_delay.value)
            }
            import aiohttp
            async with aiohttp.ClientSession() as session:
                await session.post('http://localhost:8080/api/configure', json=options)
                await session.post('http://localhost:8080/api/settings/save')
            ui.notify('Settings saved!')

        ui.button('Save Settings', on_click=save_settings).props('color=primary').classes('mt-4')


# ============================================================================
# DEBUG PAGE
# ============================================================================

@ui.page('/debug')
def debug_ui():
    ui.colors(primary='#1976D2')

    with ui.column().classes('w-full p-8 max-w-4xl mx-auto gap-4'):
        ui.label('Debug Console').classes('text-3xl font-bold')
        ui.button('← Back to Game', on_click=lambda: ui.navigate.to('/'))

        with ui.card().classes('w-full p-4'):
            ui.label('Event Log').classes('text-xl font-bold mb-2')
            event_log = ui.log().classes('w-full h-64')

        with ui.card().classes('w-full p-4'):
            ui.label('Game State').classes('text-xl font-bold mb-2')
            state_display = ui.json_editor({'content': {'json': {}}}).classes('w-full')

        async def update_debug():
            import aiohttp
            async with aiohttp.ClientSession() as session:
                while True:
                    try:
                        async with session.get('http://localhost:8080/api/state') as resp:
                            state = await resp.json()
                            state_display.update({'content': {'json': state}})
                            for e in state.get('events', [])[-5:]:
                                event_log.push(e)
                    except Exception:
                        pass
                    await asyncio.sleep(0.5)

        ui.timer(0.5, update_debug, once=False)


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
