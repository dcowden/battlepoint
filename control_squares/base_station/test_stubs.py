from battlepoint_core import Clock,GameEvent, CooldownTimer,   EventManager, Team,ControlPoint,Proximity,TeamColor
# test_stubs.py
from battlepoint_core import Team

class MockClock(Clock):
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

class MockEventManager:
    """No-op version of EventManager, like the C TestEventManager."""
    def __init__(self):
        self.events = []

    def init(self, *args, **kwargs):
        pass

    def control_point_being_captured(self, team: Team):
        pass

    def control_point_contested(self):
        pass

    def control_point_captured(self, team: Team):
        pass

    def starting_game(self):
        pass

    def game_started(self):
        pass

    def victory(self, team: Team):
        pass

    def overtime(self):
        pass

    def cancelled(self):
        pass

    def starts_in_seconds(self, secs: int):
        pass

    def ends_in_seconds(self, secs: int):
        pass

    def get_events(self, limit: int = 100):
        return []


class MockControlPoint:
    """
    Dumb test control point, like the C one:
      - you set owner/capturing/on manually
      - update() does nothing
      - percent captured is whatever you set
    """
    def __init__(self):
        self._owner = Team.NOBODY
        self._capturing = Team.NOBODY
        self._on = Team.NOBODY
        self._percent_captured = 0

    def init(self, seconds_to_capture: int):
        # C test just calls init, it doesn't expect behavior here
        self._percent_captured = 0
        self._capturing = Team.NOBODY
        self._on = Team.NOBODY
        self._owner = Team.NOBODY

    # ---- setters to simulate game state ----
    def set_owner(self, team: Team):
        self._owner = team

    def set_capturing_team(self, team: Team):
        self._capturing = team

    def set_on(self, team: Team):
        self._on = team

    def set_percent_captured(self, pct: int):
        self._percent_captured = pct

    # ---- getters the games/tests use ----
    def get_owner(self) -> Team:
        return self._owner

    def get_capturing(self) -> Team:
        return self._capturing

    def get_on(self) -> Team:
        return self._on

    def is_owned_by(self, team: Team) -> bool:
        return self._owner == team

    def get_capture_progress_percent(self) -> float:
        # tests compare ints, but float is fine
        return float(self._percent_captured)

    # ---- no-op update (important!) ----
    def update(self, *args, **kwargs):
        # real ControlPoint updates proximity;
        # this test double must NOT do that
        pass
