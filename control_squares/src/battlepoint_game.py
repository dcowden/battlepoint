"""
BattlePoint - Real-life Team Fortress 2 Game
Python/NiceGUI Implementation
"""

from enum import Enum
from dataclasses import dataclass
from typing import Optional, List, Callable
import time
import asyncio

from nicegui import ui, app


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
    """Get color for a team"""
    if team == Team.NOBODY:
        return TeamColor.BLACK
    elif team == Team.BLU:
        return TeamColor.BLUE
    elif team == Team.RED:
        return TeamColor.RED
    else:
        return TeamColor.AQUA

def team_text(team: Team) -> str:
    """Get text representation of team"""
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
    """Get single character for team"""
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
    """Get text representation of game mode"""
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
    """Abstract clock interface"""
    def milliseconds(self) -> int:
        raise NotImplementedError

    def seconds_since(self, start_millis: int) -> int:
        return (self.milliseconds() - start_millis) // 1000


class RealClock(Clock):
    """Real-time clock using system time"""
    def milliseconds(self) -> int:
        return int(time.time() * 1000)


class TestClock(Clock):
    """Test clock for testing purposes"""
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
    """Represents a Bluetooth tag"""
    id: int
    tag_type: TagType
    team: Team = Team.NOBODY
    last_seen: int = 0
    rssi: int = 0  # Signal strength

    def is_active(self, current_time: int, timeout_ms: int = 1000) -> bool:
        """Check if tag is currently active based on last seen time"""
        return (current_time - self.last_seen) < timeout_ms


class BluetoothScanner:
    """Scans for Bluetooth tags and processes game data"""
    def __init__(self, clock: Clock):
        self.clock = clock
        self.tags: dict[int, BluetoothTag] = {}
        self.service_uuid = "YOUR_SERVICE_UUID_HERE"
        self._scanning = False

    async def start_scanning(self):
        """Start scanning for tags"""
        self._scanning = True
        # TODO: Implement actual BLE scanning
        # Example structure:
        # from bleak import BleakScanner
        #
        # async with BleakScanner() as scanner:
        #     devices = await scanner.discover()
        #     for device in devices:
        #         if self.service_uuid in device.metadata.get('uuids', []):
        #             await self.process_tag(device)

    async def stop_scanning(self):
        """Stop scanning"""
        self._scanning = False

    def parse_tag_data(self, raw_data: bytes) -> Optional[BluetoothTag]:
        """Parse raw Bluetooth data into tag structure"""
        # Example format: [tag_type, tag_id_high, tag_id_low, team, ...additional_data]
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
        """Get all active tags, optionally filtered by type"""
        current_time = self.clock.milliseconds()
        active = [tag for tag in self.tags.values() if tag.is_active(current_time)]

        if tag_type:
            active = [tag for tag in active if tag.tag_type == tag_type]

        return active

    def get_players_near_control_point(self, control_point_id: int) -> dict[Team, int]:
        """Count players near a specific control point"""
        # In real implementation, this would check proximity based on RSSI
        # and control point location
        counts = {Team.RED: 0, Team.BLU: 0}

        for tag in self.get_active_tags(TagType.PLAYER):
            if tag.team in [Team.RED, Team.BLU]:
                counts[tag.team] += 1

        return counts


# ============================================================================
# COOLDOWN TIMER
# ============================================================================

class CooldownTimer:
    """Timer that prevents actions from happening too frequently"""
    def __init__(self, interval_ms: int, clock: Clock):
        self.interval_ms = interval_ms
        self.clock = clock
        self.last_event = -interval_ms  # Start in ready state

    def can_run(self) -> bool:
        """Check if enough time has passed to run again"""
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
    """Configuration for a game"""
    mode: GameMode = GameMode.KOTH
    capture_seconds: int = 20
    capture_button_threshold_seconds: int = 5
    time_limit_seconds: int = 60
    start_delay_seconds: int = 5

    def time_limit_millis(self) -> int:
        return self.time_limit_seconds * 1000

    def validate(self):
        """Ensure settings are valid"""
        if self.capture_seconds >= self.time_limit_seconds:
            self.capture_seconds = self.time_limit_seconds - 1
        if self.capture_button_threshold_seconds >= self.capture_seconds:
            self.capture_button_threshold_seconds = self.capture_seconds - 1


# ============================================================================
# SIMPLE METER (for testing and internal use)
# ============================================================================

class SimpleMeter:
    """Simple meter that tracks a value between 0 and max"""
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
# PROXIMITY DETECTION
# ============================================================================

class Proximity:
    """Tracks which players are near control points via Bluetooth tags"""
    def __init__(self, game_options: GameOptions, clock: Clock):
        self.game_options = game_options
        self.clock = clock
        self.last_red_press = -1
        self.last_blu_press = -1

    def red_button_press(self):
        """Called when red button is pressed"""
        self.last_red_press = self.clock.milliseconds()

    def blu_button_press(self):
        """Called when blue button is pressed"""
        self.last_blu_press = self.clock.milliseconds()

    def update(self, red_close: bool, blu_close: bool):
        """Update proximity based on direct sensor readings"""
        if red_close:
            self.red_button_press()
        if blu_close:
            self.blu_button_press()

    def is_red_close(self) -> bool:
        """Check if red team is currently close"""
        if self.last_red_press < 0:
            return False
        threshold_ms = self.game_options.capture_button_threshold_seconds * 1000
        return (self.clock.milliseconds() - self.last_red_press) < threshold_ms

    def is_blu_close(self) -> bool:
        """Check if blue team is currently close"""
        if self.last_blu_press < 0:
            return False
        threshold_ms = self.game_options.capture_button_threshold_seconds * 1000
        return (self.clock.milliseconds() - self.last_blu_press) < threshold_ms

    def is_team_close(self, team: Team) -> bool:
        """Check if specified team is close"""
        if team == Team.RED:
            return self.is_red_close()
        elif team == Team.BLU:
            return self.is_blu_close()
        return False


# ============================================================================
# EVENT MANAGER
# ============================================================================

class EventManager:
    """Manages game events and sound triggers"""
    def __init__(self, clock: Clock):
        self.clock = clock
        self.capture_timer = CooldownTimer(20000, clock)
        self.contest_timer = CooldownTimer(20000, clock)
        self.overtime_timer = CooldownTimer(5000, clock)
        self.start_time_timer = CooldownTimer(900, clock)
        self.end_time_timer = CooldownTimer(900, clock)
        self.cp_alert_interval_ms = 5000
        self.events = []  # List of events for UI

    def init(self, cp_alert_interval_seconds: int):
        self.cp_alert_interval_ms = cp_alert_interval_seconds * 1000

    def _add_event(self, event: str):
        """Add event to log"""
        self.events.append(event)
        print(f"[EVENT] {event}")

    def control_point_being_captured(self, team: Team):
        if self.capture_timer.can_run():
            self._add_event(f"Control Point Being Captured by {team_text(team)}")

    def control_point_contested(self):
        if self.contest_timer.can_run():
            self._add_event("Control Point is Contested!")

    def control_point_captured(self, team: Team):
        self._add_event(f"Control Point Captured by {team_text(team)}")

    def starting_game(self):
        self._add_event("Starting Game")

    def game_started(self):
        self._add_event("Game Started")

    def victory(self, team: Team):
        self._add_event(f"Victory for {team_text(team)}!")

    def overtime(self):
        if self.overtime_timer.can_run():
            self._add_event("OVERTIME!")

    def cancelled(self):
        self._add_event("Game Cancelled")

    def starts_in_seconds(self, secs: int):
        if self.start_time_timer.can_run():
            if secs in [30, 20, 10, 5, 4, 3, 2, 1]:
                self._add_event(f"Game starts in {secs} seconds")

    def ends_in_seconds(self, secs: int):
        if self.end_time_timer.can_run():
            if secs in [120, 60, 30, 20, 10, 5, 4, 3, 2, 1]:
                self._add_event(f"Game ends in {secs} seconds")


# ============================================================================
# CONTROL POINT
# ============================================================================

class ControlPoint:
    """Represents a capturable control point"""
    def __init__(self, event_manager: EventManager, clock: Clock):
        self.event_manager = event_manager
        self.clock = clock
        self._owner = Team.NOBODY
        self._on = Team.NOBODY
        self._capturing = Team.NOBODY
        self._value = 0  # milliseconds of capture progress
        self._contested = False
        self._seconds_to_capture = 20
        self._last_update_time = 0
        self._enable_red_capture = True
        self._enable_blu_capture = True
        self._should_contest_message = True

    def init(self, seconds_to_capture: int):
        """Initialize the control point for a new game"""
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
        """Check if point is fully captured and stable"""
        return (self._owner != Team.NOBODY and
                (self._on == Team.NOBODY or self._on == self._owner) and
                self._value <= 0)

    def captured_by(self, team: Team) -> bool:
        return self.captured() and self._owner == team

    def set_owner(self, team: Team):
        """Directly set owner (for testing)"""
        self._owner = team

    def update(self, proximity: Proximity):
        """Update control point state based on proximity"""
        red_on = proximity.is_red_close()
        blu_on = proximity.is_blu_close()

        self._contested = False
        is_one_team_on = False

        # Determine who is on the point
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

        # Check for start of capture
        if self._capturing == Team.NOBODY and is_one_team_on:
            if self._owner == Team.NOBODY or self._on != self._owner:
                can_capture = ((self._on == Team.RED and self._enable_red_capture) or
                              (self._on == Team.BLU and self._enable_blu_capture))
                if can_capture:
                    self._capturing = self._on
                    self.event_manager.control_point_being_captured(self._capturing)

        # Update capture progress
        millis_since_last = self.clock.milliseconds() - self._last_update_time

        if is_one_team_on:
            if self._capturing == self._on:
                self._inc_capture(millis_since_last)
            elif self._capturing != self._on:
                self._dec_capture(millis_since_last)
        elif self._on == Team.NOBODY:
            self._dec_capture(millis_since_last)

        # Check if capture is complete
        self._check_capture()
        self._last_update_time = self.clock.milliseconds()

    def _inc_capture(self, millis: int):
        """Increase capture progress"""
        self._value += millis
        capture_ms = self._seconds_to_capture * 1000
        if self._value > capture_ms:
            self._value = capture_ms

    def _dec_capture(self, millis: int):
        """Decrease capture progress"""
        self._value -= millis
        if self._value <= 0:
            self._capturing = Team.NOBODY
            self._value = 0

    def _check_capture(self):
        """Check if capture is complete"""
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
        """Get capture progress as percentage (0-100)"""
        if self._seconds_to_capture == 0:
            return 0
        return (self._value / (self._seconds_to_capture * 1000)) * 100


# ============================================================================
# BASE GAME CLASS
# ============================================================================

class Game:
    """Base game class"""
    def __init__(self):
        self.control_point: Optional[ControlPoint] = None
        self.game_options: Optional[GameOptions] = None
        self.event_manager: Optional[EventManager] = None
        self.proximity: Optional[Proximity] = None
        self.clock: Optional[Clock] = None
        self._winner = Team.NOBODY
        self._start_time = 0
        self._last_update_time = 0
        self._red_millis = 0
        self._blu_millis = 0
        self._should_announce_overtime = True

    def init(self, control_point: ControlPoint, game_options: GameOptions,
             event_manager: EventManager, owner_meter: SimpleMeter,
             capture_meter: SimpleMeter, timer1: SimpleMeter,
             timer2: SimpleMeter, clock: Clock, proximity: Optional[Proximity] = None):
        """Initialize game with all dependencies"""
        self.control_point = control_point
        self.game_options = game_options
        self.event_manager = event_manager
        self.clock = clock
        self.proximity = proximity  # Store proximity for updates
        self.owner_meter = owner_meter
        self.capture_meter = capture_meter
        self.timer1 = timer1
        self.timer2 = timer2

    def start(self):
        """Start the game"""
        self._winner = Team.NOBODY
        self._red_millis = 0
        self._blu_millis = 0
        self._start_time = self.clock.milliseconds()
        self._last_update_time = self.clock.milliseconds()
        self._should_announce_overtime = True
        self.control_point.init(self.game_options.capture_seconds)
        self.event_manager.init(self.game_options.capture_button_threshold_seconds)
        self.event_manager.game_started()

        # Initialize meters
        self.timer1.set_max_value(self.game_options.time_limit_seconds)
        self.timer2.set_max_value(self.game_options.time_limit_seconds)
        self.owner_meter.set_to_min()
        self.capture_meter.set_to_min()

    def update(self):
        """Update game state - override in subclasses"""
        raise NotImplementedError

    def end(self):
        """End/cancel the game"""
        self._winner = Team.NOBODY
        self.event_manager.cancelled()

    def is_over(self) -> bool:
        return self._winner != Team.NOBODY

    def is_running(self) -> bool:
        return not self.is_over()

    def get_winner(self) -> Team:
        return self._winner

    def get_seconds_elapsed(self) -> int:
        return (self.clock.milliseconds() - self._start_time) // 1000

    def get_remaining_seconds(self) -> int:
        elapsed = self.get_seconds_elapsed()
        return max(0, self.game_options.time_limit_seconds - elapsed)

    def get_accumulated_seconds(self, team: Team) -> int:
        if team == Team.RED:
            return self._red_millis // 1000
        elif team == Team.BLU:
            return self._blu_millis // 1000
        return 0

    def _end_game(self, winner: Team):
        """Internal method to end game"""
        self._winner = winner
        self.event_manager.victory(winner)


# ============================================================================
# GAME MODE IMPLEMENTATIONS
# ============================================================================

class KothGame(Game):
    """King of the Hill game mode"""
    def update(self):
        if self.is_over():
            return

        self.control_point.update(self.proximity)

        millis_since_last = self.clock.milliseconds() - self._last_update_time
        red_seconds_left = self.get_remaining_seconds_for_team(Team.RED)
        blu_seconds_left = self.get_remaining_seconds_for_team(Team.BLU)

        # Accumulate time for team owning the point
        if self.control_point.get_owner() == Team.RED:
            self._red_millis += millis_since_last
        elif self.control_point.get_owner() == Team.BLU:
            self._blu_millis += millis_since_last

        # Check for game over and overtime
        if red_seconds_left <= 0:
            if self.control_point.captured_by(Team.RED):
                self._end_game(Team.RED)
                return
            else:
                if self._should_announce_overtime:
                    self.event_manager.overtime()
                    self._should_announce_overtime = False

        if blu_seconds_left <= 0:
            if self.control_point.captured_by(Team.BLU):
                self._end_game(Team.BLU)
                return
            else:
                if self._should_announce_overtime:
                    self.event_manager.overtime()
                    self._should_announce_overtime = False

        # Announce time remaining
        if self.control_point.get_owner() == Team.BLU:
            self.event_manager.ends_in_seconds(blu_seconds_left)
        elif self.control_point.get_owner() == Team.RED:
            self.event_manager.ends_in_seconds(red_seconds_left)

        self._last_update_time = self.clock.milliseconds()

    def get_remaining_seconds_for_team(self, team: Team) -> int:
        time_limit_ms = self.game_options.time_limit_millis()
        if team == Team.RED:
            return (time_limit_ms - self._red_millis) // 1000
        elif team == Team.BLU:
            return (time_limit_ms - self._blu_millis) // 1000
        return 0


class ADGame(Game):
    """Attack/Defend game mode"""
    def start(self):
        super().start()
        # In AD mode, Red starts owning the point, Blue must capture
        self.control_point.set_owner(Team.RED)
        self.control_point.set_red_capture(False)  # Red cannot capture

    def update(self):
        if self.is_over():
            return

        self.control_point.update(self.proximity)

        millis_since_last = self.clock.milliseconds() - self._last_update_time
        self._red_millis += millis_since_last
        self._blu_millis += millis_since_last

        red_seconds_left = self.get_remaining_seconds()

        # Blue wins if they capture the point
        if self.control_point.captured_by(Team.BLU):
            self._end_game(Team.BLU)
            return

        # Check time limit
        if red_seconds_left <= 0:
            # If blue is capturing, overtime
            if self.control_point.get_capturing() == Team.BLU:
                if self._should_announce_overtime:
                    self.event_manager.overtime()
                    self._should_announce_overtime = False
            else:
                # Red wins by defending
                self._end_game(Team.RED)
                return

        self.event_manager.ends_in_seconds(red_seconds_left)
        self._last_update_time = self.clock.milliseconds()


class CPGame(Game):
    """Control Point game mode - team with most capture time wins"""
    def update(self):
        if self.is_over():
            return

        self.control_point.update(self.proximity)

        millis_since_last = self.clock.milliseconds() - self._last_update_time

        # Accumulate time for team owning the point
        if self.control_point.get_owner() == Team.RED:
            self._red_millis += millis_since_last
        elif self.control_point.get_owner() == Team.BLU:
            self._blu_millis += millis_since_last

        # Check if time limit reached
        current_time = self.clock.milliseconds()
        if (current_time - self._start_time) // 1000 > self.game_options.time_limit_seconds:
            # Winner is team with most accumulated time
            if self._red_millis > self._blu_millis:
                self._end_game(Team.RED)
            else:
                self._end_game(Team.BLU)
            return

        self._last_update_time = self.clock.milliseconds()


# ============================================================================
# GAME FACTORY
# ============================================================================

def create_game(mode: GameMode) -> Game:
    """Factory to create the appropriate game type"""
    if mode == GameMode.KOTH:
        return KothGame()
    elif mode == GameMode.AD:
        return ADGame()
    elif mode == GameMode.CP:
        return CPGame()
    return KothGame()



# ============================================================================
# GAME BACKEND
# ============================================================================

class GameBackend:
    """Main game backend that runs independently"""
    def __init__(self):
        self.clock = RealClock()
        self.game_options = GameOptions()
        self.event_manager = EventManager(self.clock)
        self.proximity = Proximity(self.game_options, self.clock)
        self.control_point = ControlPoint(self.event_manager, self.clock)
        self.scanner = BluetoothScanner(self.clock)  # Fixed: pass clock

        # Meters (SimpleMeter for backend)
        self.owner_meter = SimpleMeter()
        self.capture_meter = SimpleMeter()
        self.timer1 = SimpleMeter()
        self.timer2 = SimpleMeter()

        self.game: Optional[Game] = None
        self._running = False

    def configure(self, options: GameOptions):
        """Update game options"""
        self.game_options = options
        self.game_options.validate()

    def start_game(self):
        """Start a new game"""
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
            self.proximity  # Pass proximity
        )
        self.game.start()
        self._running = True

    def stop_game(self):
        """Stop/cancel current game"""
        if self.game:
            self.game.end()
        self._running = False

    def update(self):
        """Update game state - call this frequently"""
        if self.game and self._running:
            self.game.update()
            if self.game.is_over():
                self._running = False

    def get_state(self) -> dict:
        """Get current game state for frontend"""
        if not self.game:
            return {'running': False, 'winner': None}

        # compute frozen remaining_seconds if not running
        if not self._running:
            # freeze at _last_update_time, not now
            elapsed_ms = max(0, self.game._last_update_time - self.game._start_time)
            remaining_seconds = max(0, self.game_options.time_limit_seconds - (elapsed_ms // 1000))
        else:
            remaining_seconds = self.game.get_remaining_seconds()

        return {
            'running': self._running,
            'winner': team_text(self.game.get_winner()) if self.game.is_over() else None,
            'elapsed_seconds': self.game.get_seconds_elapsed() if self._running else (
                    self.game._last_update_time - self.game._start_time) // 1000,
            'remaining_seconds': remaining_seconds,
            'control_point': {
                'owner': team_text(self.control_point.get_owner()),
                'capturing': team_text(self.control_point.get_capturing()),
                'on': team_text(self.control_point.get_on()),
                'contested': self.control_point.is_contested(),
                'progress': self.control_point.get_capture_progress_percent()
            },
            'red_accumulated': self.game.get_accumulated_seconds(Team.RED),
            'blu_accumulated': self.game.get_accumulated_seconds(Team.BLU),
            'events': self.event_manager.events[-10:],
        }


# ============================================================================
# FASTAPI SETUP
# ============================================================================

backend = GameBackend()
_game_loop_task = None

async def game_loop():
    """Background task to update game state"""
    while True:
        backend.update()
        await asyncio.sleep(0.1)  # 100ms update rate

# API endpoints using NiceGUI's app.get/post decorators
@app.get("/api/state")
async def get_state():
    """Get current game state"""
    return backend.get_state()

@app.post("/api/start")
async def start_game_endpoint():
    """Start a new game"""
    backend.start_game()
    return {"status": "started"}

@app.post("/api/stop")
async def stop_game_endpoint():
    """Stop current game"""
    backend.stop_game()
    return {"status": "stopped"}

@app.post("/api/configure")
async def configure(options: dict):
    """Configure game options"""
    go = GameOptions(
        mode=GameMode(options.get('mode', 0)),
        capture_seconds=options.get('capture_seconds', 20),
        capture_button_threshold_seconds=options.get('capture_button_threshold_seconds', 5),
        time_limit_seconds=options.get('time_limit_seconds', 60),
        start_delay_seconds=options.get('start_delay_seconds', 5)
    )
    backend.configure(go)
    return {"status": "configured"}

@app.post("/api/proximity/red")
async def red_proximity():
    """Simulate red team proximity"""
    backend.proximity.red_button_press()
    return {"status": "ok"}

@app.post("/api/proximity/blue")
async def blue_proximity():
    """Simulate blue team proximity"""
    backend.proximity.blu_button_press()
    return {"status": "ok"}


# ============================================================================
# NICEGUI FRONTEND
# ============================================================================

# Frontend pages
@ui.page('/')
async def game_ui():
    """Main game UI page"""

    ui.colors(primary='#1976D2')

    ui.add_head_html("""
    <script>
      window.BP = window.BP || {};
      BP.play = (freq=880, ms=180) => {
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

    with ui.column().classes('w-full h-screen'):
        # Header
        with ui.row().classes('w-full justify-between items-center p-4 bg-primary'):
            ui.label('BattlePoint').classes('text-2xl font-bold text-white')
            ui.button('Settings', on_click=lambda: ui.navigate.to('/settings')).props('flat color=white')

        # Main game area
        with ui.row().classes('w-full flex-grow'):
            # Left - Red timer
            with ui.column().classes('w-1/4 p-4'):
                ui.label('RED TEAM').classes('text-xl font-bold text-center')
                red_timer = ui.linear_progress(value=0).props('color=red size=30px')
                red_time_label = ui.label('0:00').classes('text-3xl text-center mt-4')

            # Center - Game clock and status
            with ui.column().classes('w-1/2 p-4 items-center justify-center'):
                game_clock = ui.label('0:00').classes('text-[14rem] leading-none font-bold text-center')
                status_label = ui.label('Waiting...').classes('text-2xl text-center mt-4')

                # Control point status
                with ui.card().classes('w-full mt-8 p-4'):
                    ui.label('Control Point').classes('text-xl font-bold text-center')
                    cp_owner = ui.label('Owner: ---').classes('text-center')
                    cp_capturing = ui.label('Capturing: ---').classes('text-center')
                    cp_progress = ui.linear_progress(value=0).classes('mt-2')

            # Right - Blue timer
            with ui.column().classes('w-1/4 p-4'):
                ui.label('BLUE TEAM').classes('text-xl font-bold text-center')
                blue_timer = ui.linear_progress(value=0).props('color=blue size=30px reverse')
                blue_time_label = ui.label('0:00').classes('text-3xl text-center mt-4')

        # Bottom - Seesaw meter
        with ui.row().classes('w-full p-4'):
            seesaw = ui.linear_progress(value=50).props('color=purple size=20px').classes('w-full')

        # Control buttons (for testing)
        with ui.row().classes('w-full justify-center gap-4 p-4'):
            ui.button('Red Button', on_click=lambda: simulate_red()).props('color=red')
            ui.button('Blue Button', on_click=lambda: simulate_blue()).props('color=blue')
            ui.button('Start Game', on_click=lambda: start_game()).props('color=green')
            ui.button('Stop Game', on_click=lambda: stop_game()).props('color=orange')

    async def simulate_red():
        await ui.run_javascript('fetch("/api/proximity/red", {method: "POST"})')

    async def simulate_blue():
        await ui.run_javascript('fetch("/api/proximity/blue", {method: "POST"})')

    async def start_game():
        await ui.run_javascript('fetch("/api/start", {method: "POST"})')

    async def stop_game():
        await ui.run_javascript('fetch("/api/stop", {method: "POST"})')

    # Update UI periodically
    async def update_ui():
        while True:

            import aiohttp
            async with aiohttp.ClientSession() as session:
                async with session.get('http://localhost:8080/api/state') as resp:
                    state = await resp.json()

                    # Update labels
                    game_clock.set_text(f"{state['remaining_seconds'] // 60}:{state['remaining_seconds'] % 60:02d}")
                    cp_owner.set_text(f"Owner: {state['control_point']['owner']}")
                    cp_capturing.set_text(f"Capturing: {state['control_point']['capturing']}")
                    cp_progress.set_value(state['control_point']['progress'] / 100)

                    if state['running']:
                        status_label.set_text('GAME RUNNING')
                    elif state['winner']:
                        status_label.set_text(f"Winner: {state['winner']}")
                    else:
                        status_label.set_text('Waiting...')

                    # Update timers based on game mode
                    red_time_label.set_text(f"{state['red_accumulated'] // 60}:{state['red_accumulated'] % 60:02d}")
                    blue_time_label.set_text(f"{state['blu_accumulated'] // 60}:{state['blu_accumulated'] % 60:02d}")

            # (after you set labels/progress/etc)
            # Play simple tones for new events
            new_events = state.get('events', [])
            if hasattr(update_ui, "_last_events"):
                delta = [e for e in new_events if e not in update_ui._last_events]
            else:
                delta = new_events
            for e in delta:
                # crude mapping
                if "Captured" in e:
                    await ui.run_javascript("BP.play(660,200)")
                elif "Contested" in e:
                    await ui.run_javascript("BP.play(440,200)")
                elif "OVERTIME" in e:
                    await ui.run_javascript("BP.play(880,400)")
                elif "Victory" in e:
                    await ui.run_javascript("BP.play(523,500)")
            update_ui._last_events = new_events


            await asyncio.sleep(0.1)

    ui.timer(0.1, update_ui, once=False)


@ui.page('/settings')
def settings_ui():
    """Settings page"""

    ui.colors(primary='#1976D2')

    with ui.column().classes('w-full p-8 max-w-2xl mx-auto'):
        ui.label('Game Settings').classes('text-3xl font-bold mb-4')
        ui.button('← Back to Game', on_click=lambda: ui.navigate.to('/')).classes('mb-4')

        # Game mode selection
        mode_select = ui.select(
            ['KOTH', 'AD', 'CP'],
            label='Game Mode',
            value='KOTH'
        ).classes('w-full')

        # Time settings
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
            ui.notify('Settings saved!')

        ui.button('Save Settings', on_click=save_settings).props('color=primary').classes('mt-4')


@ui.page('/debug')
def debug_ui():
    """Debug page for testing and monitoring"""

    ui.colors(primary='#1976D2')

    with ui.column().classes('w-full p-8 max-w-4xl mx-auto'):
        ui.label('Debug Console').classes('text-3xl font-bold mb-4')
        ui.button('← Back to Game', on_click=lambda: ui.navigate.to('/')).classes('mb-4')

        # Event log
        with ui.card().classes('w-full p-4 mb-4'):
            ui.label('Event Log').classes('text-xl font-bold mb-2')
            event_log = ui.log().classes('w-full h-64')

        # State display
        with ui.card().classes('w-full p-4 mb-4'):
            ui.label('Game State').classes('text-xl font-bold mb-2')
            state_display = ui.json_editor({'content': {'json': {}}}).classes('w-full')

        # Manual controls
        with ui.card().classes('w-full p-4'):
            ui.label('Manual Controls').classes('text-xl font-bold mb-2')
            with ui.row().classes('gap-2'):
                ui.button('Red Proximity', on_click=lambda: simulate_red())
                ui.button('Blue Proximity', on_click=lambda: simulate_blue())
                ui.button('Both Teams', on_click=lambda: simulate_both())
                ui.button('Clear All', on_click=lambda: clear_proximity())

        async def simulate_red():
            import aiohttp
            async with aiohttp.ClientSession() as session:
                await session.post('http://localhost:8080/api/proximity/red')
            event_log.push('Red team proximity activated')

        async def simulate_blue():
            import aiohttp
            async with aiohttp.ClientSession() as session:
                await session.post('http://localhost:8080/api/proximity/blue')
            event_log.push('Blue team proximity activated')

        async def simulate_both():
            await simulate_red()
            await simulate_blue()

        async def clear_proximity():
            event_log.push('Proximity cleared (wait for timeout)')

        # Update debug display
        async def update_debug():
            while True:
                try:
                    import aiohttp
                    async with aiohttp.ClientSession() as session:
                        async with session.get('http://localhost:8080/api/state') as resp:
                            state = await resp.json()
                            state_display.update({'content': {'json': state}})

                            # Add new events to log
                            for event in state.get('events', [])[-5:]:
                                event_log.push(event)
                except:
                    pass

                await asyncio.sleep(0.5)

        ui.timer(0.5, update_debug, once=False)


# ============================================================================
# ADDITIONAL GAME CLASSES FOR TESTING
# ============================================================================

class TestControlPoint:
    """Mock control point for testing"""
    def __init__(self):
        self._owner = Team.NOBODY
        self._capturing = Team.NOBODY
        self._on = Team.NOBODY

    def init(self, seconds_to_capture: int):
        pass

    def update(self, proximity):
        pass

    def set_owner(self, team: Team):
        self._owner = team

    def get_owner(self) -> Team:
        return self._owner

    def get_capturing(self) -> Team:
        return self._capturing

    def get_on(self) -> Team:
        return self._on

    def set_capturing_team(self, team: Team):
        self._capturing = team

    def is_owned_by(self, team: Team) -> bool:
        return self._owner == team

    def captured_by(self, team: Team) -> bool:
        return self.is_owned_by(team)

    def set_red_capture(self, enabled: bool):
        """Allow red capture (for AD mode)"""
        pass

    def set_blu_capture(self, enabled: bool):
        """Allow blue capture (for AD mode)"""
        pass


class TestEventManager:
    """Mock event manager for testing"""
    def __init__(self):
        self.events = []

    def init(self, interval: int):
        pass

    def control_point_being_captured(self, team: Team):
        self.events.append(f"CP being captured by {team_text(team)}")

    def control_point_contested(self):
        self.events.append("CP contested")

    def control_point_captured(self, team: Team):
        self.events.append(f"CP captured by {team_text(team)}")

    def starting_game(self):
        self.events.append("Starting game")

    def game_started(self):
        self.events.append("Game started")

    def victory(self, team: Team):
        self.events.append(f"Victory: {team_text(team)}")

    def overtime(self):
        self.events.append("OVERTIME")

    def cancelled(self):
        self.events.append("Game cancelled")

    def starts_in_seconds(self, secs: int):
        pass

    def ends_in_seconds(self, secs: int):
        pass



# ============================================================================
# SETTINGS PERSISTENCE
# ============================================================================

import json
import os

class SettingsManager:
    """Manages saving and loading game settings"""
    def __init__(self, filename: str = "battlepoint_settings.json"):
        self.filename = filename

    def save_settings(self, options: GameOptions, volume: int = 10, brightness: int = 50) -> bool:
        """Save settings to file"""
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
        """Load settings from file"""
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
    """Manages game sounds and audio playback"""
    def __init__(self):
        self.enabled = True
        self.volume = 10
        # In production, initialize audio library (pygame, sounddevice, etc.)

    def play(self, sound_id: int):
        """Play a sound by ID"""
        if not self.enabled:
            return

        # Sound ID mapping from original C code
        sound_map = {
            1: "announcer_alert.mp3",
            2: "announcer_alert_center_control_being_contested.mp3",
            3: "announcer_last_flag.mp3",
            12: "announcer_victory.mp3",
            14: "announcer_we_captured_control.mp3",
            17: "announcer_overtime2.mp3",
            # ... add more mappings
        }

        sound_file = sound_map.get(sound_id)
        if sound_file:
            print(f"[SOUND] Playing: {sound_file}")
            # In production:
            # pygame.mixer.music.load(f"sounds/{sound_file}")
            # pygame.mixer.music.play()

    def set_volume(self, volume: int):
        """Set volume (0-30)"""
        self.volume = max(0, min(30, volume))
        # In production: pygame.mixer.music.set_volume(self.volume / 30)

    def stop(self):
        """Stop all sounds"""
        # In production: pygame.mixer.music.stop()
        pass


# ============================================================================
# ENHANCED GAME BACKEND WITH ALL FEATURES
# ============================================================================

class EnhancedGameBackend(GameBackend):
    """Extended backend with sound, settings, and Bluetooth"""
    def __init__(self):
        super().__init__()
        self.sound_system = SoundSystem()
        self.settings_manager = SettingsManager()
        self.bluetooth_scanner = BluetoothScanner(self.clock)

        # Load saved settings
        saved = self.settings_manager.load_settings()
        if saved:
            options, volume, brightness = saved
            self.configure(options)
            self.sound_system.set_volume(volume)

    def save_settings(self):
        """Save current settings"""
        return self.settings_manager.save_settings(
            self.game_options,
            self.sound_system.volume
        )

    async def start_bluetooth_scanning(self):
        """Start Bluetooth scanning"""
        await self.bluetooth_scanner.start_scanning()

    async def stop_bluetooth_scanning(self):
        """Stop Bluetooth scanning"""
        await self.bluetooth_scanner.stop_scanning()

    def start_game_with_countdown(self) -> List[int]:
        """Start game with countdown, return countdown steps"""
        countdown_seconds = self.game_options.start_delay_seconds
        return list(range(countdown_seconds, 0, -1))


# ============================================================================
# ENHANCED FASTAPI ENDPOINTS
# ============================================================================

enhanced_backend = EnhancedGameBackend()

@app.post("/api/settings/save")
async def save_settings():
    """Save current settings to disk"""
    success = enhanced_backend.save_settings()
    return {"status": "saved" if success else "error"}

@app.get("/api/settings/load")
async def load_settings():
    """Load settings from disk"""
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
    """Play a sound"""
    enhanced_backend.sound_system.play(sound_id)
    return {"status": "playing"}

@app.post("/api/sound/volume/{volume}")
async def set_volume(volume: int):
    """Set sound volume"""
    enhanced_backend.sound_system.set_volume(volume)
    return {"volume": enhanced_backend.sound_system.volume}

@app.get("/api/bluetooth/tags")
async def get_bluetooth_tags():
    """Get all active Bluetooth tags"""
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
    """Start Bluetooth scanning"""
    await enhanced_backend.start_bluetooth_scanning()
    return {"status": "scanning"}

@app.post("/api/bluetooth/stop")
async def stop_bluetooth():
    """Stop Bluetooth scanning"""
    await enhanced_backend.stop_bluetooth_scanning()
    return {"status": "stopped"}


# ============================================================================
# ENHANCED GAME UI WITH MORE FEATURES
# ============================================================================

@ui.page('/game')
async def enhanced_game_ui():
    """Enhanced game UI with all features"""

    ui.colors(primary='#1976D2')

    # Add custom CSS for animations
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
            .meter-container {
                position: relative;
                height: 40px;
                background: #333;
                border-radius: 20px;
                overflow: hidden;
            }
            .meter-fill {
                height: 100%;
                transition: width 0.3s ease;
                border-radius: 20px;
            }
        </style>
    ''')

    with ui.column().classes('w-full h-screen bg-gray-900 text-white'):
        # Header with navigation
        with ui.row().classes('w-full justify-between items-center p-4 bg-gray-800'):
            ui.label('🎮 BattlePoint').classes('text-3xl font-bold')
            with ui.row().classes('gap-2'):
                ui.button('Settings', on_click=lambda: ui.navigate.to('/settings'), icon='settings').props('flat')
                ui.button('Debug', on_click=lambda: ui.navigate.to('/debug'), icon='bug_report').props('flat')

        # Main game display
        with ui.row().classes('w-full flex-grow p-4 gap-4'):
            # RED TEAM SIDE
            with ui.column().classes('w-1/5 gap-4'):
                ui.label('RED TEAM').classes('text-2xl font-bold text-red-500 text-center')
                with ui.card().classes('bg-gray-800 p-4'):
                    red_time = ui.label('0:00').classes('text-4xl font-mono text-center')
                    ui.separator()
                    red_progress = ui.linear_progress(0).props('color=red size=30px')
                ui.space()

            # CENTER GAME AREA
            with ui.column().classes('w-3/5 gap-4 items-center justify-center'):
                # Main game clock
                with ui.card().classes('bg-gray-800 p-8 w-full'):
                    game_status = ui.label('Ready to Start').classes('text-xl text-center text-gray-400 mb-4')
                    main_clock = ui.label('0:00').classes('text-9xl font-mono font-bold text-center')

                # Control Point Status
                with ui.card().classes('bg-gray-800 p-6 w-full'):
                    ui.label('⚔️ CONTROL POINT').classes('text-2xl font-bold text-center mb-4')

                    with ui.row().classes('w-full justify-between mb-2'):
                        cp_owner_label = ui.label('Owner: ---').classes('text-lg')
                        cp_capturing_label = ui.label('Capturing: ---').classes('text-lg')

                    cp_progress_bar = ui.linear_progress(0).props('size=20px color=purple').classes('mb-2')
                    cp_status_label = ui.label('Neutral').classes('text-center text-gray-400')

                # Quick actions
                with ui.row().classes('gap-2 mt-4'):
                    start_btn = ui.button('▶ START GAME', on_click=lambda: start_game()).props('color=green size=lg')
                    stop_btn = ui.button('⏹ STOP', on_click=lambda: stop_game()).props('color=orange size=lg')
                    stop_btn.set_visibility(False)

            # BLUE TEAM SIDE
            with ui.column().classes('w-1/5 gap-4'):
                ui.label('BLUE TEAM').classes('text-2xl font-bold text-blue-500 text-center')
                with ui.card().classes('bg-gray-800 p-4'):
                    blue_time = ui.label('0:00').classes('text-4xl font-mono text-center')
                    ui.separator()
                    blue_progress = ui.linear_progress(0).props('color=blue size=30px reverse')
                ui.space()

        # Bottom Seesaw Meter
        with ui.row().classes('w-full p-4 bg-gray-800'):
            with ui.column().classes('w-full'):
                ui.label('⚖️ TEAM BALANCE').classes('text-center text-gray-400 mb-2')
                seesaw_meter = ui.linear_progress(0.5).props('size=30px').classes('w-full')
                with ui.row().classes('w-full justify-between text-sm text-gray-400 mt-1'):
                    ui.label('◀ RED')
                    ui.label('BLUE ▶')

        # Event Feed (bottom corner)
        with ui.card().classes('fixed bottom-4 right-4 w-80 bg-gray-800 p-4 max-h-60 overflow-auto'):
            ui.label('📋 Event Feed').classes('text-lg font-bold mb-2')
            event_feed = ui.column().classes('gap-1')

    # Game control functions
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
        game_status.set_text('Ready to Start')
        game_status.classes('text-gray-400', remove='text-green-400')

    # Real-time UI updates
    async def update_ui_loop():
        last_events = []

        while True:
            try:
                import aiohttp
                async with aiohttp.ClientSession() as session:
                    async with session.get('http://localhost:8080/api/state') as resp:
                        state = await resp.json()

                        # Update main clock
                        mins = state['remaining_seconds'] // 60
                        secs = state['remaining_seconds'] % 60
                        main_clock.set_text(f'{mins}:{secs:02d}')

                        # Update team times
                        red_mins = state['red_accumulated'] // 60
                        red_secs = state['red_accumulated'] % 60
                        red_time.set_text(f'{red_mins}:{red_secs:02d}')

                        blue_mins = state['blu_accumulated'] // 60
                        blue_secs = state['blu_accumulated'] % 60
                        blue_time.set_text(f'{blue_mins}:{blue_secs:02d}')

                        # Update control point
                        cp = state['control_point']
                        cp_owner_label.set_text(f'Owner: {cp["owner"]}')
                        cp_capturing_label.set_text(f'Capturing: {cp["capturing"]}')
                        cp_progress_bar.set_value(cp['progress'] / 100)

                        if cp['contested']:
                            cp_status_label.set_text('⚠️ CONTESTED!')
                            cp_status_label.classes('text-yellow-400 pulse')
                        else:
                            cp_status_label.set_text(f'Status: {cp["owner"]}')
                            cp_status_label.classes('text-gray-400', remove='text-yellow-400 pulse')

                        # Update seesaw meter (balance between teams)
                        total = state['red_accumulated'] + state['blu_accumulated']
                        if total > 0:
                            balance = state['blu_accumulated'] / total
                        else:
                            balance = 0.5
                        seesaw_meter.set_value(balance)

                        # Update event feed
                        new_events = state.get('events', [])
                        if new_events != last_events:
                            event_feed.clear()
                            for event in reversed(new_events[-5:]):
                                with event_feed:
                                    ui.label(f'• {event}').classes('text-sm text-gray-300')
                            last_events = new_events

                        # Check game status
                        if state['winner']:
                            game_status.set_text(f'🏆 {state["winner"]} WINS!')
                            game_status.classes('text-yellow-400 flash')
                            start_btn.set_visibility(True)
                            stop_btn.set_visibility(False)
                        elif state['running']:
                            game_status.set_text('🎮 GAME IN PROGRESS')
                            game_status.classes('text-green-400')

                # Play simple tones for new events
                if hasattr(update_ui_loop, "_last_events"):
                    delta = [e for e in new_events if e not in update_ui_loop._last_events]
                else:
                    delta = new_events
                for e in delta:
                    if "Captured" in e:
                        await ui.run_javascript("BP.play(660,200)")
                    elif "Contested" in e:
                        await ui.run_javascript("BP.play(440,200)")
                    elif "OVERTIME" in e:
                        await ui.run_javascript("BP.play(880,400)")
                    elif "Victory" in e:
                        await ui.run_javascript("BP.play(523,500)")
                update_ui_loop._last_events = new_events

            except Exception as e:
                print(f"UI update error: {e}")

            await asyncio.sleep(0.1)

    ui.timer(0.1, update_ui_loop, once=False)


if __name__ in {"__main__", "__mp_main__"}:
    # Use enhanced backend
    backend = enhanced_backend

    # Start the game loop
    async def start_game_loop():
        """Start the background game update loop"""
        global _game_loop_task
        if _game_loop_task is None:
            _game_loop_task = asyncio.create_task(game_loop())

    # Schedule game loop to start after UI is ready
    app.on_startup(start_game_loop)

    ui.run(title='BattlePoint - TF2 Game', port=8080, reload=False)