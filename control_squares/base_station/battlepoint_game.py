from battlepoint_core import GameOptions, EventManager, Clock, LedMeter, Proximity,Team, ControlPoint,TeamColor, GameMode,get_team_color
# battlepoint_game.py

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
)


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
        self._last_end_announce_second = -1  # (won't be used after the change, but safe)

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
        # just like C: timer max = time limit
        self.timer1.setMaxValue(self.options.time_limit_seconds)
        self.timer2.setMaxValue(self.options.time_limit_seconds)

        self.control_point.init(self.options.capture_seconds)
        self.game_type_init()
        self.update_display()

    def start(self):
        # C: start() calls resetGame()
        self.reset_game()
        self._start_time_ms = self.clock.milliseconds()
        if self.events:
            self.events.game_started()

    def end(self):
        # C: end() -> cancelled()
        self._winner = Team.NOBODY
        self._start_time_ms = self.NOT_STARTED
        if self.events:
            self.events.cancelled()

    # ------------------------------------------------------------------
    def is_running(self) -> bool:
        return self._start_time_ms != self.NOT_STARTED and self._winner == Team.NOBODY

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
        return self.options.time_limit_seconds - self.get_accumulated_seconds(team)

    def get_remaining_seconds(self) -> int:
        if self._winner != Team.NOBODY:
            return 0
        return self.get_game_type_remaining_seconds()

    def get_winner(self) -> Team:
        return self._winner

    # ------------------------------------------------------------------
    def update(self):
        if not self.is_running():
            return

        self._update_accumulated_time()

        winner = self.check_victory()
        if winner != Team.NOBODY:
            self._end_game_with_winner(winner)
            self.update_display()
            return

        # send "ends in" like C
        if self.events:
            rem = self.get_remaining_seconds()
            #print(f"Remaining seconds: {rem}")
            #if rem != self._last_end_announce_second:
            #    self.events.ends_in_seconds(rem)
            #    self._last_end_announce_second = rem
            self.events.ends_in_seconds(rem)

        self.update_display()

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
        # C does not force meters, but our update_display will run right after

    # ------------------------------------------------------------------
    # hooks
    # ------------------------------------------------------------------
    def game_type_init(self):
        pass

    def update_display(self):
        pass

    def get_game_type_remaining_seconds(self) -> int:
        # default: global countdown
        elapsed = self.get_seconds_elapsed()
        rem = self.options.time_limit_seconds - elapsed
        return rem if rem > 0 else 0

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
        # color setup (tests don’t check, but mirror C style)
        self.timer1.fgColor(TeamColor.BLUE)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.RED)
        self.timer2.bgColor(TeamColor.BLACK)

    def update_display(self):
        # like C: timer1 = blue remaining, timer2 = red remaining
        self.timer1.setValue(self.get_remaining_seconds_for_team(Team.BLU))
        self.timer2.setValue(self.get_remaining_seconds_for_team(Team.RED))

        # capture meter
        self.capture_meter.setValue(int(self.control_point.get_capture_progress_percent()))

        # owner meter
        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        self.owner_meter.bgColor(TeamColor.BLACK)
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()

    def get_game_type_remaining_seconds(self) -> int:
        owner = self.control_point.get_owner()
        if owner == Team.RED:
            return self.get_remaining_seconds_for_team(Team.RED)
        elif owner == Team.BLU:
            return self.get_remaining_seconds_for_team(Team.BLU)
        else:
            # no owner → time stays at limit
            return self.options.time_limit_seconds

    def check_victory(self) -> Team:
        if self.control_point.get_owner() == Team.BLU and self.get_remaining_seconds_for_team(Team.BLU) <= 0:
            return Team.BLU
        if self.control_point.get_owner() == Team.RED and self.get_remaining_seconds_for_team(Team.RED) <= 0:
            return Team.RED
        return Team.NOBODY

    def check_overtime(self) -> bool:
        # same as C
        if self.get_remaining_seconds_for_team(Team.BLU) <= 0 and self.control_point.get_capturing() == Team.RED:
            return True
        if self.get_remaining_seconds_for_team(Team.RED) <= 0 and self.control_point.get_capturing() == Team.BLU:
            return True
        return False


# ======================================================================
# AD (Attack / Defend)
# ======================================================================
class ADGame(BaseGame):
    def game_type_init(self):
        # red defends → starts owning
        self.control_point.set_owner(Team.RED)
        self.timer1.fgColor(TeamColor.BLUE)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.RED)
        self.timer2.bgColor(TeamColor.BLACK)

    def update_display(self):
        # we can just show remaining global time on both
        remaining = self.get_game_type_remaining_seconds()
        self.timer1.setValue(remaining)
        self.timer2.setValue(remaining)

        self.capture_meter.setValue(int(self.control_point.get_capture_progress_percent()))

        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        self.owner_meter.bgColor(TeamColor.BLACK)
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()

    def get_game_type_remaining_seconds(self) -> int:
        elapsed = self.get_seconds_elapsed()
        rem = self.options.time_limit_seconds - elapsed
        return rem if rem > 0 else 0

    def check_victory(self) -> Team:
        # blue captures → blue wins immediately
        if self.control_point.get_owner() == Team.BLU:
            return Team.BLU

        # time up → red wins
        if self.get_game_type_remaining_seconds() <= 0:
            return Team.RED

        return Team.NOBODY


# ======================================================================
# CP (Control Point, timeboxed)
# ======================================================================
class CPGame(BaseGame):
    def game_type_init(self):
        self.control_point.set_owner(Team.NOBODY)
        self.timer1.fgColor(TeamColor.BLUE)
        self.timer1.bgColor(TeamColor.BLACK)
        self.timer2.fgColor(TeamColor.RED)
        self.timer2.bgColor(TeamColor.BLACK)

    def update_display(self):
        # IMPORTANT: C tests expect at the END:
        #   timer1 == BLUE accumulated
        #   timer2 == RED accumulated
        # so just always show accumulated
        blue_sec = self.get_accumulated_seconds(Team.BLU)
        red_sec = self.get_accumulated_seconds(Team.RED)

        self.timer1.setValue(blue_sec)
        self.timer2.setValue(red_sec)

        self.capture_meter.setValue(int(self.control_point.get_capture_progress_percent()))

        owner = self.control_point.get_owner()
        self.owner_meter.fgColor(get_team_color(owner))
        self.owner_meter.bgColor(TeamColor.BLACK)
        if owner == Team.NOBODY:
            self.owner_meter.setToMin()
        else:
            self.owner_meter.setToMax()

    def get_game_type_remaining_seconds(self) -> int:
        # CP = plain countdown
        elapsed = self.get_seconds_elapsed()
        rem = self.options.time_limit_seconds - elapsed
        return rem if rem > 0 else 0

    def check_victory(self) -> Team:
        # only decide once time is up
        if self.get_game_type_remaining_seconds() > 0:
            return Team.NOBODY

        red_sec = self.get_accumulated_seconds(Team.RED)
        blue_sec = self.get_accumulated_seconds(Team.BLU)

        if red_sec > blue_sec:
            return Team.RED
        elif blue_sec > red_sec:
            return Team.BLU
        else:
            return Team.NOBODY


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
