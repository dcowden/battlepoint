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

# battlepoint_core.py (or wherever Proximity lives)
from dataclasses import dataclass
from enum import Enum
import re

MAX_PLAYERS_PER_TEAM = 3  # clamp

class Proximity:
    """
    Proximity with per-team player counts (0..3) and the same 'linger' threshold
    behavior as before.

    Back-compat:
      - update(red_close: bool, blu_close: bool) still works (1-or-0)
      - red_button_press() / blu_button_press() are kept for UI & old code:
        they simulate "one player present" for that team using the numeric path.
      - is_red_close() / is_blu_close() use effective_count > 0.
    """

    def __init__(self, game_options: GameOptions, clock: Clock):
        self.game_options = game_options
        self.clock = clock

        self._last_red_seen_ms = -1
        self._last_blu_seen_ms = -1

        self._observed_red_count = 0   # last observed (without decay)
        self._observed_blu_count = 0

    # ---- new API ----
    def update_counts(self, red_count: int, blu_count: int):
        now = self.clock.milliseconds()

        red_count = max(0, min(MAX_PLAYERS_PER_TEAM, int(red_count)))
        blu_count = max(0, min(MAX_PLAYERS_PER_TEAM, int(blu_count)))

        if red_count > 0:
            self._last_red_seen_ms = now
        if blu_count > 0:
            self._last_blu_seen_ms = now

        self._observed_red_count = red_count
        self._observed_blu_count = blu_count

    # ---- back-compat shim (bool) ----
    def update(self, red_close: bool, blu_close: bool):
        self.update_counts(1 if red_close else 0,
                           1 if blu_close else 0)

    # ---- UI button shims (used by /api/proximity/red|blue) ----
    def red_button_press(self):
        """
        Simulate a RED player on the point.
        Each call refreshes RED presence as count=1.
        If you want 'while held' behavior, call this repeatedly from the UI
        (e.g. via a timer) while the button is down.
        """
        self.update_counts(1, self.get_blu_count())

    def blu_button_press(self):
        """
        Simulate a BLU player on the point.
        Each call refreshes BLU presence as count=1.
        """
        self.update_counts(self.get_red_count(), 1)

    # ---- helpers ----
    def _threshold_ms(self) -> int:
        return max(0, int(self.game_options.capture_button_threshold_seconds * 1000))

    def get_red_count(self) -> int:
        if self._last_red_seen_ms < 0:
            return 0
        if (self.clock.milliseconds() - self._last_red_seen_ms) > self._threshold_ms():
            return 0
        return self._observed_red_count

    def get_blu_count(self) -> int:
        if self._last_blu_seen_ms < 0:
            return 0
        if (self.clock.milliseconds() - self._last_blu_seen_ms) > self._threshold_ms():
            return 0
        return self._observed_blu_count

    # ---- legacy bool API ----
    def is_red_close(self) -> bool:
        return self.get_red_count() > 0

    def is_blu_close(self) -> bool:
        return self.get_blu_count() > 0

    def is_team_close(self, team: Team) -> bool:
        if team == Team.RED:
            return self.is_red_close()
        if team == Team.BLU:
            return self.is_blu_close()
        return False




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


@dataclass
class GameEvent:
    ts_ms: int     # absolute time in ms (from your Clock)
    text: str

    def to_display(self) -> str:
        # render as "HH:MM:SS — message"
        sec = self.ts_ms / 1000.0
        lt = time.localtime(sec)
        ts = time.strftime("%H:%M:%S", lt)
        return f"{ts} — {self.text}"

class EventManager:
    def __init__(self, clock: Clock, sound_system=None):
        self.clock = clock
        self.capture_timer = CooldownTimer(20000, clock)
        self.contest_timer = CooldownTimer(20000, clock)
        self.overtime_timer = CooldownTimer(20000, clock)
        self.start_time_timer = CooldownTimer(900, clock)
        self.end_time_timer = CooldownTimer(900, clock)
        self.cp_alert_interval_ms = 5000

        # now this is a list of GameEvent, not plain str
        self.events: list[GameEvent] = []

        self.sound_system = sound_system

        # one-shot latches (per game)
        self._starts_announced: set[int] = set()
        self._ends_announced: set[int] = set()
        self._starting_game_announced: bool = False

    def init(self, cp_alert_interval_seconds: int):
        self.cp_alert_interval_ms = cp_alert_interval_seconds * 1000

    def _add_event(self, event: str):
        ev = GameEvent(ts_ms=self.clock.milliseconds(), text=event)
        self.events.append(ev)
        print(f"[EVENT] {ev.to_display()}")

    def _play(self, snd_id: int):
        if self.sound_system:
            self.sound_system.play(snd_id)

    def _reset_game_latches(self):
        """Reset one-shot sound guards at the start of each game."""
        self._starts_announced.clear()
        self._ends_announced.clear()
        self._starting_game_announced = False

    # === match C ===
    def control_point_being_captured(self, team: Team):
        if self.capture_timer.can_run():
            self._add_event(f"Control Point Being Captured by {team_text(team)}")
            self._play(3)

    def control_point_contested(self):
        if self.contest_timer.can_run():
            self._add_event("Control Point is Contested!")
            self._play(2)

    def control_point_captured(self, team: Team):
        self._add_event(f"Control Point Captured by {team_text(team)}")
        self._play(14)

    def starting_game(self):
        # treat this as "new game" boundary: reset one-shot latches
        self._reset_game_latches()

        # (optional) clear old events when a new game starts
        self.events.clear()

        self._add_event("Starting Game")

        # ensure this sound is only played once per game
        if not self._starting_game_announced:
            self._play(10)
            self._starting_game_announced = True

    def game_started(self):
        self._add_event("Game Started")
        self._play(11)

    def victory(self, team: Team):
        self._add_event(f"Victory for {team_text(team)}!")
        self._play(12)

    def overtime(self):
        if self.overtime_timer.can_run():
            self._add_event("OVERTIME!")
            self._play(6)

    def cancelled(self):
        self._add_event("Game Cancelled")
        self._play(17)
        self.sound_system.play_menu_track()

    def starts_in_seconds(self, secs: int):
        # only fire specific countdown calls once per game per second-mark
        tracked = [60, 30, 20, 10, 5, 4, 3, 2, 1]
        if secs in tracked and secs not in self._starts_announced:
            if self.start_time_timer.can_run():
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
                # latch this second so it doesn't fire again this game
                self._starts_announced.add(secs)

    def ends_in_seconds(self, secs: int):
        # only fire specific countdown calls once per game per second-mark
        tracked = [120, 60, 30, 20, 10, 5, 4, 3, 2, 1]
        if secs in tracked and secs not in self._ends_announced:
            if self.end_time_timer.can_run():
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
                # latch this second so it doesn't fire again this game
                self._ends_announced.add(secs)

    # ---- simple API from EventManager side ----
    def get_events(self, limit: int = 100) -> list[GameEvent]:
        if limit <= 0:
            return self.events[:]
        return self.events[-limit:]


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

        prev_on = self._on
        prev_capturing = self._capturing
        prev_value = self._value

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
        if millis_since_last < 0:
            millis_since_last = 0

        APPLY_DT_MAX_MS = 1000
        dt = min(millis_since_last, APPLY_DT_MAX_MS)
        if dt > APPLY_DT_MAX_MS:
            dt = APPLY_DT_MAX_MS

        capture_ms = self._seconds_to_capture * 1000

        # NEW: scale by player count (1..3)
        red_count = proximity.get_red_count()
        blu_count = proximity.get_blu_count()

        if is_one_team_on:
            if self._capturing == self._on:
                if self._on == Team.RED:
                    self._inc_capture(dt * max(1, red_count))
                else:
                    self._inc_capture(dt * max(1, blu_count))
            else:
                # reverse/decay is not amplified; we count normal time
                self._dec_capture(dt)
        elif self._on == Team.NOBODY:
            # keep your existing "finish if within last delta" logic
            if (
                prev_capturing != Team.NOBODY
                and prev_on == prev_capturing
                and prev_value > 0
            ):
                remaining = capture_ms - prev_value
                if remaining <= dt:
                    self._owner = prev_capturing
                    self._capturing = Team.NOBODY
                    self._value = 0
                    self.event_manager.control_point_captured(self._owner)
                else:
                    self._dec_capture(dt)
            else:
                self._dec_capture(dt)

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

    # _check_capture, get_* remain the same

    def _check_capture(self):
        #print(f"Check Capture: Needed {self._value}/{self._seconds_to_capture*1000}")
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



