"""
BattlePoint Unit Tests
Following the patterns from platformio_cpp_tests
"""

import pytest
from battlepoint_core import get_team_color,team_text_char,game_mode_text, CooldownTimer, BluetoothTag, TagType, EventManager, Team, TeamColor, Proximity, GameOptions, LedMeter,GameMode,team_text,RealClock,ControlPoint
from battlepoint_game import KothGame,CPGame,ADGame,BaseGame
from battlepoint_app import EnhancedBLEScanner
from test_stubs import MockEventManager,MockControlPoint, MockClock
import time



# ---------------------------------------------------------------------------
# helpers to mimic the C++ test doubles
# ---------------------------------------------------------------------------

class _TestProximity:
    """
    C++ had:
        tp.setBluClose(true);
        tp.setRedClose(true);
    and ControlPoint pulled from the proximity each update.

    This is the same thing for Python tests.
    """
    def __init__(self):
        self._red = False
        self._blu = False

    def set_red_close(self, v: bool):
        self._red = v

    def set_blu_close(self, v: bool):
        self._blu = v

    def is_red_close(self) -> bool:
        return self._red

    def is_blu_close(self) -> bool:
        return self._blu

    def get_red_count(self):
        return 0

    def get_blu_count(self):
        return 0


# ---------------------------------------------------------------------------
# TEAM UTILITY TESTS
# ---------------------------------------------------------------------------

def test_team_color_mappings():
    assert get_team_color(Team.BLU) == TeamColor.BLUE
    assert get_team_color(Team.RED) == TeamColor.RED
    assert get_team_color(Team.NOBODY) == TeamColor.BLACK
    assert get_team_color(Team.BOTH) == TeamColor.AQUA


def test_team_chars():
    assert team_text_char(Team.BLU) == "B"
    assert team_text_char(Team.RED) == "R"
    assert team_text_char(Team.NOBODY) == "-"
    assert team_text_char(Team.BOTH) == "+"


def test_team_text():
    assert team_text(Team.BLU) == "BLU"
    assert team_text(Team.RED) == "RED"
    assert team_text(Team.NOBODY) == "---"
    assert team_text(Team.BOTH) == "R&B"


def test_game_mode_text():
    assert game_mode_text(GameMode.KOTH) == "KOTH"
    assert game_mode_text(GameMode.AD) == "AD"
    assert game_mode_text(GameMode.CP) == "CP"


# ---------------------------------------------------------------------------
# CLOCK TESTS
# ---------------------------------------------------------------------------

def test_real_clock():
    c = RealClock()
    m = c.milliseconds()
    time.sleep(1)
    n = c.milliseconds()
    assert abs((n - m) - 1000) < 50  # 50ms slack


def test_mock_clock_basic():
    tc = MockClock()
    assert tc.milliseconds() == 0
    tc.set_time(200)
    assert tc.milliseconds() == 200
    time.sleep(0.2)
    # still 200 because it's manual
    assert tc.milliseconds() == 200


def test_mock_clock_add():
    tc = MockClock()
    tc.add_millis(200)
    assert tc.milliseconds() == 200
    tc.add_millis(400)
    assert tc.milliseconds() == 600


def test_clock_seconds_since():
    tc = MockClock()
    tc.add_millis(4000)
    assert tc.seconds_since(1000) == 3


# ---------------------------------------------------------------------------
# COOLDOWN TIMER TESTS
# ---------------------------------------------------------------------------

def test_basic_timer():
    clock = MockClock()
    ct = CooldownTimer(500, clock)
    assert ct.can_run()
    assert not ct.can_run()
    clock.add_millis(100)
    assert not ct.can_run()
    clock.add_millis(600)
    assert ct.can_run()
    clock.add_millis(600)
    assert ct.can_run()


# ---------------------------------------------------------------------------
# CONTROL POINT TESTS (mirror C++)
# ---------------------------------------------------------------------------

def test_control_point_initial_value_cppstyle():
    tc = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, tc)
    cp.init(5)
    prox = _TestProximity()

    # first update like C++ does
    cp.update(prox)
    assert cp.is_contested() is False
    assert cp.get_on() == Team.NOBODY
    assert cp.get_owner() == Team.NOBODY
    assert cp.get_capturing() == Team.NOBODY
    assert cp.get_capture_progress_percent() == 0


def test_control_point_basic_blu_capture():
    tc = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, tc)
    prox = _TestProximity()

    cp.init(1)  # 1 second to capture, same as C++
    prox.set_blu_close(True)

    cp.update(prox)
    tc.add_millis(200)
    cp.update(prox)

    pct = cp.get_capture_progress_percent()
    assert 1 <= pct <= 25  # C++ did INT_WITHIN(1,20,...)

    assert cp.get_on() == Team.BLU
    assert cp.is_contested() is False
    assert cp.get_owner() == Team.NOBODY
    assert cp.get_capturing() == Team.BLU

    # finish capture
    tc.add_millis(900)
    cp.update(prox)

    # after capture it resets to 0 just like C++
    assert cp.get_capture_progress_percent() == 0
    assert cp.get_on() == Team.BLU
    assert cp.get_owner() == Team.BLU
    assert cp.get_capturing() == Team.NOBODY


def test_control_point_contested_cppstyle():
    tc = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, tc)
    prox = _TestProximity()

    cp.init(1)
    prox.set_blu_close(True)
    prox.set_red_close(True)

    cp.update(prox)
    tc.add_millis(200)
    cp.update(prox)

    assert cp.get_capture_progress_percent() == 0
    assert cp.get_on() == Team.BOTH
    assert cp.is_contested()
    assert cp.get_owner() == Team.NOBODY
    assert cp.get_capturing() == Team.NOBODY


def test_control_point_count_back_down():
    tc = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, tc)
    prox = _TestProximity()

    cp.init(1)
    prox.set_blu_close(True)

    cp.update(prox)
    tc.add_millis(200)
    cp.update(prox)
    pct = cp.get_capture_progress_percent()
    assert 1 <= pct <= 25

    # now leave
    prox.set_blu_close(False)
    tc.add_millis(100)
    cp.update(prox)
    assert cp.get_capture_progress_percent() == 10  # 0.1s left

    tc.add_millis(400)
    cp.update(prox)
    assert cp.get_capture_progress_percent() == 0
    assert cp.get_on() == Team.NOBODY
    assert cp.get_owner() == Team.NOBODY
    assert cp.get_capturing() == Team.NOBODY
    assert not cp.is_contested()


def test_control_point_capture_disabled():
    tc = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, tc)
    prox = _TestProximity()

    cp.init(1)
    cp.set_blu_capture(False)
    cp.set_red_capture(False)

    prox.set_red_close(True)

    cp.update(prox)
    tc.add_millis(200)
    cp.update(prox)

    assert cp.get_on() == Team.RED
    assert not cp.is_contested()
    assert cp.get_capturing() == Team.NOBODY
    assert cp.get_capture_progress_percent() == 0


def test_control_point_reverse_capture():
    tc = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, tc)
    prox = _TestProximity()

    cp.init(1)
    cp.set_blu_capture(True)
    cp.set_red_capture(True)

    # point is owned by BLU, RED walks on -> reverse
    cp.set_owner(Team.BLU)
    prox.set_red_close(True)
    cp.update(prox)
    tc.add_millis(200)
    cp.update(prox)

    pct = cp.get_capture_progress_percent()
    assert 1 <= pct <= 25
    assert cp.get_on() == Team.RED
    assert cp.get_owner() == Team.BLU
    assert cp.get_capturing() == Team.RED
    assert not cp.is_contested()

    # now BLU also steps on -> contested, no inc/dec
    prox.set_blu_close(True)
    cp.update(prox)
    tc.add_millis(100)
    cp.update(prox)
    pct2 = cp.get_capture_progress_percent()
    assert cp.is_contested()
    assert pct2 == pct  # no change while contested

    # RED steps off -> goes back down
    prox.set_red_close(False)
    cp.update(prox)
    tc.add_millis(100)
    cp.update(prox)
    assert cp.get_capture_progress_percent() < pct2


# ---------------------------------------------------------------------------
# PROXIMITY TESTS (your original ones, still valid)
# ---------------------------------------------------------------------------

def test_proximity_initially_not_close():
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    tc = MockClock()
    bp = Proximity(go, tc)

    assert not bp.is_blu_close()
    assert not bp.is_red_close()
    assert not bp.is_team_close(Team.RED)
    assert not bp.is_team_close(Team.BLU)


def test_proximity_red_close():
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    tc = MockClock()
    bp = Proximity(go, tc)

    bp.update(True, False)
    assert bp.is_red_close()
    assert not bp.is_blu_close()

    tc.add_millis(400)
    assert bp.is_red_close()
    tc.add_millis(400)
    assert bp.is_red_close()
    tc.add_millis(400)
    assert not bp.is_red_close()


def test_proximity_both_teams():
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    tc = MockClock()
    bp = Proximity(go, tc)

    bp.update(True, True)
    assert bp.is_red_close()
    assert bp.is_blu_close()


# ---------------------------------------------------------------------------
# GAME OPTIONS TESTS
# ---------------------------------------------------------------------------

def test_game_options_defaults():
    go = GameOptions()
    assert go.mode == GameMode.KOTH
    assert go.capture_seconds == 20
    assert go.time_limit_seconds == 60


def test_game_options_validation():
    go = GameOptions(capture_seconds=70, time_limit_seconds=60)
    go.validate()
    assert go.capture_seconds < go.time_limit_seconds


# ---------------------------------------------------------------------------
# KOTH GAME TESTS (mirror C++)
# ---------------------------------------------------------------------------

def _koth_game_options():
    return GameOptions(
        mode=GameMode.KOTH,
        time_limit_seconds=2,
        capture_seconds=1,
        capture_button_threshold_seconds=1,
        start_delay_seconds=0,
    )


def test_koth_game_initial_state_cppstyle():
    go = _koth_game_options()
    game = KothGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)

    assert not game.is_running()
    assert game.get_winner() == Team.NOBODY
    assert game.get_seconds_elapsed() == 0
    assert game.get_remaining_seconds() == go.time_limit_seconds
    assert owner.getValue() == 0
    assert capture.getValue() == 0


def test_koth_game_keeps_time_cppstyle():
    go = _koth_game_options()
    game = KothGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    cp.set_owner(Team.NOBODY)
    game.start()

    assert game.get_winner() == Team.NOBODY
    assert game.is_running()
    assert game.get_remaining_seconds() == go.time_limit_seconds

    tc.add_millis(1000)
    game.update()
    assert abs(game.get_seconds_elapsed() - 1) <= 1

    # still full since nobody owns
    assert game.get_remaining_seconds() == go.time_limit_seconds
    assert game.get_accumulated_seconds(Team.RED) == 0
    assert game.get_accumulated_seconds(Team.BLU) == 0



def test_koth_game_ends_after_capture_cppstyle():
    go = _koth_game_options()
    game = KothGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    cp.set_owner(Team.BLU)
    game.update()

    tc.add_millis(1100)
    game.update()

    assert game.get_accumulated_seconds(Team.RED) == 0
    assert game.get_accumulated_seconds(Team.BLU) == 1

    tc.add_millis(1000)
    game.update()

    assert game.get_winner() == Team.BLU
    assert game.get_remaining_seconds() == 0

    # like C++
    assert owner.getValue() == 100
    assert cp.get_owner() == Team.BLU
    assert game.get_remaining_seconds_for_team(Team.RED) == go.time_limit_seconds
    assert game.get_remaining_seconds_for_team(Team.BLU) == 0
    assert game.get_accumulated_seconds(Team.RED) == 0
    assert game.get_accumulated_seconds(Team.BLU) == 2
    assert timer2.getValue() == go.time_limit_seconds
    assert timer1.getValue() == 0


# ---------------------------------------------------------------------------
# AD GAME TESTS (mirror C++)
# ---------------------------------------------------------------------------

def _std_game_options():
    return GameOptions(
        mode=GameMode.CP,  # C++ uses CP as "std" for AD/CP tests
        time_limit_seconds=20,
        start_delay_seconds=0,
        capture_seconds=5,
        capture_button_threshold_seconds=1,
    )


def test_ad_game_initial_state_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.AD

    game = ADGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.update()  # C++ calls update once

    assert not game.is_running()
    assert game.get_winner() == Team.NOBODY
    assert game.get_seconds_elapsed() == 0
    assert game.get_remaining_seconds() == go.time_limit_seconds
    assert cp.get_owner() == Team.RED  # red defends
    assert owner.getValue() == 100
    assert capture.getValue() == 0


def test_ad_game_keeps_time_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.AD

    game = ADGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    assert game.get_winner() == Team.NOBODY
    assert game.is_running()
    assert game.get_remaining_seconds() == go.time_limit_seconds
    assert cp.get_owner() == Team.RED

    CAP_TIME = 5
    tc.add_seconds(CAP_TIME)
    game.update()

    assert abs(game.get_seconds_elapsed() - CAP_TIME) <= 1
    assert game.get_remaining_seconds() == go.time_limit_seconds - CAP_TIME
    assert game.get_accumulated_seconds(Team.RED) == CAP_TIME
    assert game.get_accumulated_seconds(Team.BLU) == 0


def test_ad_game_ends_after_capture_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.AD

    game = ADGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    cp.set_owner(Team.BLU)
    tc.add_seconds(1)
    game.update()

    assert game.get_winner() == Team.BLU
    assert not game.is_running()
    assert owner.getValue() == 100
    assert cp.get_owner() == Team.BLU
    assert game.get_remaining_seconds() == 0


def test_ad_game_ends_after_time_limit_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.AD

    game = ADGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    tc.add_seconds(go.time_limit_seconds + 1)
    game.update()

    assert game.get_winner() == Team.RED
    assert not game.is_running()
    assert owner.getValue() == 100
    assert cp.get_owner() == Team.RED
    assert game.get_remaining_seconds() == 0


# ---------------------------------------------------------------------------
# CP GAME TESTS (mirror C++)
# ---------------------------------------------------------------------------

def test_cp_game_initial_state_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.CP

    game = CPGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)

    assert not game.is_running()
    assert game.get_winner() == Team.NOBODY
    assert game.get_seconds_elapsed() == 0
    assert game.get_remaining_seconds() == go.time_limit_seconds
    assert owner.getValue() == 0
    assert capture.getValue() == 0


def test_cp_game_keeps_time_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.CP

    game = CPGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    assert game.get_winner() == Team.NOBODY
    assert game.is_running()
    assert cp.get_owner() == Team.NOBODY

    CAP_TIME = 5
    tc.add_seconds(CAP_TIME)
    cp.set_owner(Team.RED)
    game.update()

    assert abs(game.get_seconds_elapsed() - CAP_TIME) <= 1
    assert game.get_remaining_seconds() == go.time_limit_seconds - CAP_TIME
    assert game.get_accumulated_seconds(Team.RED) == CAP_TIME
    assert game.get_accumulated_seconds(Team.BLU) == 0


def test_cp_game_ends_after_time_limit_cppstyle():
    go = _std_game_options()
    go.mode = GameMode.CP

    game = CPGame()
    tc = MockClock()
    cp = MockControlPoint()
    em = MockEventManager()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Timer1", 100)
    timer2 = LedMeter("Timer2", 100)

    game.init(cp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    BLUE_CAP_TIME = 5
    RED_CAP_TIME = 20

    cp.set_owner(Team.BLU)
    tc.add_seconds(BLUE_CAP_TIME)
    game.update()

    cp.set_owner(Team.RED)
    tc.add_seconds(RED_CAP_TIME)
    game.update()

    assert game.get_accumulated_seconds(Team.RED) == RED_CAP_TIME
    assert game.get_accumulated_seconds(Team.BLU) == BLUE_CAP_TIME
    assert game.get_winner() == Team.RED
    assert not game.is_running()
    assert owner.getValue() == 100
    assert cp.get_owner() == Team.RED
    assert game.get_remaining_seconds() == 0
    # display meters
    assert timer2.getValue() == RED_CAP_TIME
    assert timer1.getValue() == BLUE_CAP_TIME


# ---------------------------------------------------------------------------
# INTEGRATION-LIKE TEST (like your Python one)
# ---------------------------------------------------------------------------

def test_full_game_flow_like_python():
    # this stays KOTH, but we simulate capture like the C++ tests do:
    go = GameOptions(
        mode=GameMode.KOTH,
        time_limit_seconds=10,
        capture_seconds=2,
        capture_button_threshold_seconds=1,
        start_delay_seconds=0,
    )

    tc = MockClock()
    em = EventManager(tc)
    em.init(go.capture_button_threshold_seconds)

    # IMPORTANT: use mock CP, not real CP, so the game loop doesn't undo our capture
    cp = MockControlPoint()

    owner = LedMeter("Owner", 100)
    capture = LedMeter("Capture", 100)
    timer1 = LedMeter("Red", 100)
    timer2 = LedMeter("Blue", 100)

    game = KothGame()
    game.init(cp, go, em, owner, capture, timer1, timer2, tc)

    # start the game
    game.start()
    assert game.is_running()
    assert game.get_winner() == Team.NOBODY

    # after 3 seconds we "pretend" the point was captured by RED
    tc.add_seconds(3)
    cp.set_owner(Team.RED)        # exactly what the C++ tests do with TestControlPoint
    game.update()

    assert cp.get_owner() == Team.RED

    # run out the rest of the time
    tc.add_seconds(10)
    game.update()

    assert game.is_over()
    assert game.get_winner() == Team.RED

def test_proximity_counts_and_decay():
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    clk = MockClock()
    p = Proximity(go, clk)

    p.update_counts(2, 0)
    assert p.get_red_count() == 2
    assert p.is_red_close()
    assert not p.is_blu_close()

    # after 900 ms still within threshold
    clk.add_millis(900)
    assert p.get_red_count() == 2

    # after another 200 ms, decay to 0
    clk.add_millis(200)
    assert p.get_red_count() == 0
    assert not p.is_red_close()

def test_control_point_capture_scales_with_players():
    clk = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, clk)

    go = GameOptions()
    go.capture_button_threshold_seconds = 5  # keep counts alive
    p = Proximity(go, clk)

    cp.init(1)  # 1 second total to capture

    # 1 player red for 200ms -> ~20% progress
    p.update_counts(1, 0)
    cp.update(p)
    clk.add_millis(200)
    cp.update(p)
    pct_one = cp.get_capture_progress_percent()
    assert 15 <= pct_one <= 25

    # reset CP
    cp.init(1)

    # 3 players red for 200ms -> ~60% progress
    p.update_counts(3, 0)
    cp.update(p)
    clk.add_millis(200)
    cp.update(p)
    pct_three = cp.get_capture_progress_percent()
    assert 50 <= pct_three <= 70
    assert pct_three > pct_one * 2  # should be ~3x

def test_control_point_contested_no_progress():
    clk = MockClock()
    em = MockEventManager()
    cp = ControlPoint(em, clk)

    go = GameOptions()
    go.capture_button_threshold_seconds = 5
    p = Proximity(go, clk)

    cp.init(1)
    p.update_counts(2, 1)  # both on
    cp.update(p)
    clk.add_millis(300)
    cp.update(p)

    assert cp.is_contested()
    assert cp.get_capture_progress_percent() == 0

import time

def test_ble_scanner_counts_from_cs_ads():
    clk = MockClock()
    s = EnhancedBLEScanner(clk)

    now = time.time()  # fresh timestamp

    with s._lock:
        s.devices.clear()
        s.devices["AA:BB:CC:DD:EE:01"] = {
            'name': 'CS-01',
            'rssi': -50,
            'address': 'AA:BB:CC:DD:EE:01',
            'last_seen': now,
            'manufacturer_data': 'B,CS-01,PLB-07,999',
        }
        s.devices["AA:BB:CC:DD:EE:02"] = {
            'name': 'CS-02',
            'rssi': -60,
            'address': 'AA:BB:CC:DD:EE:02',
            'last_seen': now,
            'manufacturer_data': 'R,CS-01,PLR-02,-72',
        }
        s.devices["AA:BB:CC:DD:EE:03"] = {
            'name': 'Random',
            'rssi': -40,
            'address': 'AA:BB:CC:DD:EE:03',
            'last_seen': now,  # value doesn’t matter; name isn’t CS-*
            'manufacturer_data': 'junk',
        }

    counts = s.get_player_counts()
    assert counts['blu'] == 1
    assert counts['red'] == 1



if __name__ == '__main__':
    pytest.main([__file__, '-v'])