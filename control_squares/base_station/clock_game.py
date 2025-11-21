# Simple clock-only game mode backend

from enum import Enum
from typing import Optional

from battlepoint_core import Clock, RealClock, EventManager, Team


class ClockPhase(Enum):
    IDLE = "idle"
    RUNNING = "running"
    ENDED = "ended"


class ClockBackend:
    """
    Minimal backend that just runs a countdown clock, fires EventManager
    notifications, and plays an endgame / victory sound when time expires.
    """

    def __init__(self, clock: Optional[Clock] = None, event_manager: Optional[EventManager] = None):
        self.clock: Clock = clock or RealClock()
        self.event_manager: Optional[EventManager] = event_manager or EventManager(self.clock, None)

        self._phase: ClockPhase = ClockPhase.IDLE
        self._running: bool = False

        self._time_limit_seconds: int = 600  # default 10:00
        self._remaining_seconds: int = self._time_limit_seconds

        self._start_ms: int = 0
        self._last_update_ms: int = 0

        # guard so we only fire the endgame sound once per round
        self._end_sound_fired: bool = False

    # ------------------------------------------------------------------ #
    # GAME CONTROL
    # ------------------------------------------------------------------ #

    def configure(self, time_limit_seconds: int):
        self._time_limit_seconds = max(1, int(time_limit_seconds))
        # when idle, also reset remaining to match config
        if self._phase == ClockPhase.IDLE:
            self._remaining_seconds = self._time_limit_seconds

    def start_game(self):
        self._phase = ClockPhase.RUNNING
        self._running = True
        self._remaining_seconds = self._time_limit_seconds
        now = self.clock.milliseconds()
        self._start_ms = now
        self._last_update_ms = now
        self._end_sound_fired = False

        if self.event_manager:
            self.event_manager.game_started()

    def stop_game(self):
        # manual stop – no victory sound here
        self._phase = ClockPhase.ENDED
        self._running = False
        self._remaining_seconds = 0

        if self.event_manager:
            self.event_manager.cancelled()

    # ------------------------------------------------------------------ #
    # UPDATE LOOP
    # ------------------------------------------------------------------ #

    def update(self):
        """Call this regularly (e.g. every 100 ms) from the app game loop."""
        if self._phase != ClockPhase.RUNNING:
            return

        now = self.clock.milliseconds()
        if now < self._last_update_ms:
            self._last_update_ms = now

        elapsed_ms = now - self._start_ms
        elapsed_s = max(0, elapsed_ms // 1000)

        remaining = max(0, self._time_limit_seconds - elapsed_s)
        self._remaining_seconds = remaining
        self._last_update_ms = now

        if self.event_manager:
            # generic countdown announcements; you probably already mapped these
            self.event_manager.ends_in_seconds(remaining)

        # *** NEW BEHAVIOR: when time hits 0, fire victory(Team.NOBODY) ***
        if remaining <= 0:
            if self._phase == ClockPhase.RUNNING:
                self._phase = ClockPhase.ENDED
                self._running = False

                if self.event_manager and not self._end_sound_fired:
                    # This should map to your "0023" endgame victory sound.
                    self.event_manager.victory(Team.NOBODY)
                    self._end_sound_fired = True

    # ------------------------------------------------------------------ #
    # STATE FOR API/UI
    # ------------------------------------------------------------------ #

    def get_state(self) -> dict:
        return {
            "running": self._running,
            "phase": self._phase.value,
            "time_limit_seconds": self._time_limit_seconds,
            "remaining_seconds": self._remaining_seconds,
            "winner": None,  # clock mode has no winner
        }
