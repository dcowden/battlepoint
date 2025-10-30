"""
BattlePoint Unit Tests
Following the patterns from platformio_cpp_tests
"""

import pytest
from battlepoint_game import (
    Team, GameMode, TeamColor,
    get_team_color, team_text, team_text_char, game_mode_text, BluetoothScanner,
    Clock, RealClock, TestClock,
    CooldownTimer, SimpleMeter, Proximity, EventManager,
    ControlPoint, GameOptions, Game, KothGame, ADGame, CPGame,
    TestControlPoint, TestEventManager
)
import time


# ============================================================================
# TEAM UTILITY TESTS
# ============================================================================

def test_team_color_mappings():
    """Test team color mappings"""
    assert get_team_color(Team.BLU) == TeamColor.BLUE
    assert get_team_color(Team.RED) == TeamColor.RED
    assert get_team_color(Team.NOBODY) == TeamColor.BLACK
    assert get_team_color(Team.BOTH) == TeamColor.AQUA


def test_team_chars():
    """Test team character representations"""
    assert team_text_char(Team.BLU) == 'B'
    assert team_text_char(Team.RED) == 'R'
    assert team_text_char(Team.NOBODY) == '-'
    assert team_text_char(Team.BOTH) == '+'


def test_team_text():
    """Test team text representations"""
    assert team_text(Team.BLU) == "BLU"
    assert team_text(Team.RED) == "RED"
    assert team_text(Team.NOBODY) == "---"
    assert team_text(Team.BOTH) == "R&B"


def test_game_mode_text():
    """Test game mode text"""
    assert game_mode_text(GameMode.KOTH) == "KOTH"
    assert game_mode_text(GameMode.AD) == "AD"
    assert game_mode_text(GameMode.CP) == "CP"


# ============================================================================
# CLOCK TESTS
# ============================================================================

def test_real_clock():
    """Test real clock"""
    c = RealClock()
    m = c.milliseconds()
    time.sleep(1)
    n = c.milliseconds()
    assert abs((n - m) - 1000) < 50  # Allow 50ms tolerance


def test_fake_clock():
    """Test fake clock"""
    tc = TestClock()
    assert tc.milliseconds() == 0
    tc.set_time(200)
    assert tc.milliseconds() == 200
    time.sleep(0.4)
    assert tc.milliseconds() == 200  # Doesn't advance on its own


def test_fake_clock_add():
    """Test fake clock add methods"""
    tc = TestClock()
    assert tc.milliseconds() == 0
    tc.add_millis(200)
    assert tc.milliseconds() == 200
    tc.add_millis(400)
    assert tc.milliseconds() == 600


def test_clock_seconds_since():
    """Test seconds_since calculation"""
    tc = TestClock()
    assert tc.milliseconds() == 0
    tc.add_millis(4000)
    assert tc.seconds_since(1000) == 3


# ============================================================================
# COOLDOWN TIMER TESTS
# ============================================================================

def test_basic_timer():
    """Test basic cooldown timer"""
    clock = TestClock()
    ct = CooldownTimer(500, clock)
    assert ct.can_run()  # First call should succeed
    assert not ct.can_run()  # Second immediate call should fail
    clock.add_millis(100)
    assert not ct.can_run()  # Still in cooldown
    clock.add_millis(600)
    assert ct.can_run()  # Now enough time has passed
    clock.add_millis(600)
    assert ct.can_run()  # Should work again


# ============================================================================
# SIMPLE METER TESTS
# ============================================================================

def test_simple_meter_initial_state():
    """Test simple meter initialization"""
    sm = SimpleMeter()
    assert sm.get_max_value() == 100
    assert sm.get_value() == 0


def test_simple_meter_set_values():
    """Test simple meter value setting"""
    sm = SimpleMeter()
    sm.set_value(50)
    assert sm.get_value() == 50

    sm.set_max_value(200)
    assert sm.get_max_value() == 200
    assert sm.get_value() == 50

    sm.set_to_min()
    assert sm.get_value() == 0

    sm.set_to_max()
    assert sm.get_value() == 200


def test_simple_meter_bounds():
    """Test simple meter stays within bounds"""
    sm = SimpleMeter()
    sm.set_value(150)  # Exceeds max
    assert sm.get_value() == 100  # Clamped to max

    sm.set_value(-50)  # Below min
    assert sm.get_value() == 0  # Clamped to min


# ============================================================================
# PROXIMITY TESTS
# ============================================================================

def test_proximity_initially_not_close():
    """Test proximity initial state"""
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    tc = TestClock()
    bp = Proximity(go, tc)

    assert not bp.is_blu_close()
    assert not bp.is_red_close()
    assert not bp.is_team_close(Team.RED)
    assert not bp.is_team_close(Team.BLU)


def test_proximity_red_close():
    """Test red team proximity"""
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    tc = TestClock()
    bp = Proximity(go, tc)

    bp.update(True, False)
    assert bp.is_red_close()
    assert not bp.is_blu_close()

    tc.add_millis(400)
    assert bp.is_red_close()
    assert not bp.is_blu_close()

    tc.add_millis(400)
    assert bp.is_red_close()
    assert not bp.is_blu_close()

    tc.add_millis(400)
    assert not bp.is_red_close()  # Timeout
    assert not bp.is_blu_close()


def test_proximity_both_teams():
    """Test both teams proximity"""
    go = GameOptions()
    go.capture_button_threshold_seconds = 1
    tc = TestClock()
    bp = Proximity(go, tc)

    bp.update(True, True)
    assert bp.is_red_close()
    assert bp.is_blu_close()


# ============================================================================
# CONTROL POINT TESTS
# ============================================================================

def test_control_point_initial_state():
    """Test control point initialization"""
    tc = TestClock()
    em = TestEventManager()
    cp = ControlPoint(em, tc)

    cp.init(20)
    assert cp.get_owner() == Team.NOBODY
    assert cp.get_capturing() == Team.NOBODY
    assert cp.get_on() == Team.NOBODY
    assert cp.get_value() == 0


def test_control_point_capture_progress():
    """Test control point capture progression"""
    tc = TestClock()
    em = TestEventManager()
    cp = ControlPoint(em, tc)
    go = GameOptions(capture_seconds=5)

    cp.init(go.capture_seconds)
    prox = Proximity(go, tc)

    # Red team approaches
    prox.red_button_press()
    cp.update(prox)
    assert cp.get_capturing() == Team.RED

    # Progress capture
    tc.add_millis(2000)
    cp.update(prox)
    assert cp.get_capturing() == Team.RED
    assert cp.get_value() > 0

    # Complete capture
    tc.add_millis(4000)
    cp.update(prox)
    assert cp.get_owner() == Team.RED
    assert cp.get_capturing() == Team.NOBODY


def test_control_point_contested():
    """Test control point contested state"""
    tc = TestClock()
    em = TestEventManager()
    cp = ControlPoint(em, tc)
    go = GameOptions()

    cp.init(go.capture_seconds)
    prox = Proximity(go, tc)

    # Both teams approach
    prox.red_button_press()
    prox.blu_button_press()
    cp.update(prox)

    assert cp.get_on() == Team.BOTH
    assert cp.is_contested()
    assert cp.get_capturing() == Team.NOBODY


def test_control_point_decay():
    """Test capture progress decay"""
    tc = TestClock()
    em = TestEventManager()
    cp = ControlPoint(em, tc)
    go = GameOptions(capture_seconds=5)

    cp.init(go.capture_seconds)
    prox = Proximity(go, tc)

    # Red starts capturing
    prox.red_button_press()
    cp.update(prox)
    tc.add_millis(2000)
    cp.update(prox)
    value_after_capture = cp.get_value()
    assert value_after_capture > 0

    # Red leaves
    tc.add_millis(2000)  # Expire proximity
    cp.update(prox)

    # Progress should decay
    tc.add_millis(1000)
    cp.update(prox)
    assert cp.get_value() < value_after_capture


# ============================================================================
# GAME OPTIONS TESTS
# ============================================================================

def test_game_options_defaults():
    """Test game options default values"""
    go = GameOptions()
    assert go.mode == GameMode.KOTH
    assert go.capture_seconds == 20
    assert go.time_limit_seconds == 60


def test_game_options_validation():
    """Test game options validation"""
    go = GameOptions(
        capture_seconds=70,  # Invalid: exceeds time limit
        time_limit_seconds=60
    )
    go.validate()
    assert go.capture_seconds < go.time_limit_seconds


# ============================================================================
# KOTH GAME TESTS
# ============================================================================

def test_koth_game_initial_state():
    """Test KOTH game initialization"""
    go = GameOptions(
        mode=GameMode.KOTH,
        time_limit_seconds=2,
        capture_seconds=1,
        start_delay_seconds=0
    )

    game = KothGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)

    assert not game.is_running()
    assert game.get_winner() == Team.NOBODY
    assert game.get_seconds_elapsed() == 0
    assert game.get_remaining_seconds() == go.time_limit_seconds


def test_koth_game_keeps_time():
    """Test KOTH game time tracking"""
    go = GameOptions(
        mode=GameMode.KOTH,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = KothGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    tcp.set_owner(Team.NOBODY)
    game.start()

    assert game.get_winner() == Team.NOBODY
    assert game.is_running()
    assert game.get_remaining_seconds() == go.time_limit_seconds

    tc.add_seconds(5)
    game.update()
    assert abs(game.get_seconds_elapsed() - 5) <= 1

    # In KOTH with no owner, time remaining stays at limit
    assert game.get_remaining_seconds() == go.time_limit_seconds
    assert game.get_accumulated_seconds(Team.RED) == 0
    assert game.get_accumulated_seconds(Team.BLU) == 0


def test_koth_game_ends_after_capture():
    """Test KOTH game ending condition"""
    go = GameOptions(
        mode=GameMode.KOTH,
        time_limit_seconds=2,
        capture_seconds=1,
        start_delay_seconds=0
    )

    game = KothGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    tcp.set_owner(Team.BLU)
    game.update()
    tc.add_seconds(1)
    game.update()

    assert game.get_accumulated_seconds(Team.RED) == 0
    assert game.get_accumulated_seconds(Team.BLU) == 1

    tc.add_seconds(1)
    game.update()

    assert game.get_winner() == Team.BLU
    assert game.get_remaining_seconds() == 0


# ============================================================================
# AD GAME TESTS
# ============================================================================

def test_ad_game_initial_state():
    """Test AD game initialization"""
    go = GameOptions(
        mode=GameMode.AD,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = ADGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    game.start()
    game.update()

    assert not game.is_over()
    assert game.get_winner() == Team.NOBODY
    assert tcp.get_owner() == Team.RED  # Red starts owning in AD


def test_ad_game_ends_after_capture():
    """Test AD game ending when blue captures"""
    go = GameOptions(
        mode=GameMode.AD,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = ADGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    tcp.set_owner(Team.BLU)
    tc.add_seconds(1)
    game.update()

    assert game.get_winner() == Team.BLU
    assert not game.is_running()


def test_ad_game_ends_after_time_limit():
    """Test AD game ending when time runs out"""
    go = GameOptions(
        mode=GameMode.AD,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = ADGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    tc.add_seconds(go.time_limit_seconds + 1)
    game.update()

    assert game.get_winner() == Team.RED  # Red wins by defending
    assert not game.is_running()


# ============================================================================
# CP GAME TESTS
# ============================================================================

def test_cp_game_initial_state():
    """Test CP game initialization"""
    go = GameOptions(
        mode=GameMode.CP,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = CPGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)

    assert not game.is_running()
    assert game.get_winner() == Team.NOBODY
    assert game.get_seconds_elapsed() == 0
    assert game.get_remaining_seconds() == go.time_limit_seconds


def test_cp_game_keeps_time():
    """Test CP game time accumulation"""
    go = GameOptions(
        mode=GameMode.CP,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = CPGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    assert game.get_winner() == Team.NOBODY
    assert game.is_running()

    CAP_TIME = 5
    tc.add_seconds(CAP_TIME)
    tcp.set_owner(Team.RED)
    game.update()

    assert abs(game.get_seconds_elapsed() - CAP_TIME) <= 1
    assert game.get_remaining_seconds() == go.time_limit_seconds - CAP_TIME
    assert game.get_accumulated_seconds(Team.RED) == CAP_TIME
    assert game.get_accumulated_seconds(Team.BLU) == 0


def test_cp_game_ends_after_time_limit():
    """Test CP game ending with winner determination"""
    go = GameOptions(
        mode=GameMode.CP,
        time_limit_seconds=20,
        capture_seconds=5,
        start_delay_seconds=0
    )

    game = CPGame()
    tc = TestClock()
    tcp = TestControlPoint()
    em = TestEventManager()

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game.init(tcp, go, em, owner, capture, timer1, timer2, tc)
    game.start()

    BLUE_CAP_TIME = 5
    RED_CAP_TIME = 20

    tcp.set_owner(Team.BLU)
    tc.add_seconds(BLUE_CAP_TIME)
    game.update()

    tcp.set_owner(Team.RED)
    tc.add_seconds(RED_CAP_TIME)
    game.update()

    assert game.get_accumulated_seconds(Team.RED) == RED_CAP_TIME
    assert game.get_accumulated_seconds(Team.BLU) == BLUE_CAP_TIME
    assert game.get_winner() == Team.RED
    assert not game.is_running()


# ============================================================================
# INTEGRATION TESTS
# ============================================================================

def test_full_game_flow():
    """Test complete game flow"""
    go = GameOptions(
        mode=GameMode.KOTH,
        time_limit_seconds=10,
        capture_seconds=2,
        capture_button_threshold_seconds=1,
        start_delay_seconds=0
    )

    tc = TestClock()
    em = EventManager(tc)
    em.init(go.capture_button_threshold_seconds)

    prox = Proximity(go, tc)
    cp = ControlPoint(em, tc)

    owner = SimpleMeter()
    capture = SimpleMeter()
    timer1 = SimpleMeter()
    timer2 = SimpleMeter()

    game = KothGame()
    game.init(cp, go, em, owner, capture, timer1, timer2, tc)

    # Start game
    game.start()
    assert game.is_running()

    # Red approaches and captures
    prox.red_button_press()
    tc.add_millis(100)
    cp.update(prox)
    game.update()

    # Complete capture
    tc.add_seconds(3)
    prox.red_button_press()  # Keep presence
    cp.update(prox)
    game.update()

    assert cp.get_owner() == Team.RED

    # Run out time limit
    tc.add_seconds(10)
    cp.update(prox)
    game.update()

    assert game.is_over()
    assert game.get_winner() == Team.RED


if __name__ == '__main__':
    pytest.main([__file__, '-v'])