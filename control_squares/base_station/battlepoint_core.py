from enum import Enum
from dataclasses import dataclass
import time

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



