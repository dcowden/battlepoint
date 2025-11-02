from battlepoint_core import GameOptions, EventManager, Clock, LedMeter, Proximity,Team, ControlPoint,TeamColor, GameMode,get_team_color

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
