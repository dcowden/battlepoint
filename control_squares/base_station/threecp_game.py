"""
3CP (Three Control Point) Game Mode
Linear progression: RED captures A→B→C, BLU captures C→B→A
"""

from typing import Optional, List, Dict
from enum import Enum
import traceback
import asyncio
import threading

from battlepoint_core import (
    Team,
    TeamColor,
    get_team_color,
    ControlPoint,
    EventManager,
    Clock,
    RealClock,
    Proximity,
    team_text,
    GameOptions,
)
from battlepoint_game import BaseGame
from settings import ThreeCPOptions, ControlSquareMapping
from ble_scanner import EnhancedBLEScanner


class CPOwnerState(Enum):
    """Ownership state for a control point in 3CP."""
    RED = 1
    BLU = 2
    NEUTRAL = 3


class ThreeCPGame(BaseGame):
    """
    3CP game mode with linear progression.

    Layout: [RED: A] ━━━ [NEUTRAL: B] ━━━ [BLU: C]

    Rules:
    - RED starts owning A, BLU starts owning C, B is neutral
    - RED must capture A→B→C to win
    - BLU must capture C→B→A to win
    - Only adjacent points can be captured (can't skip middle)
    - Global countdown timer (not per-team accumulation)
    - Time added on first capture of any CP


    Also note: this game mode supports magnetic captures: if a control point has an Owner,
    then any magnetic user is presumed to be the non owning team
    """

    def __init__(self):
        super().__init__()
        self.control_points: List[ControlPoint] = []
        self.cp_owners: List[CPOwnerState] = [CPOwnerState.NEUTRAL] * 3
        self.cp_first_capture: List[bool] = [False, False, False]  # Track if CP captured for first time
        self.time_added_total: int = 0
        self.options_3cp: Optional[ThreeCPOptions] = None

    def init_3cp(
        self,
        control_points: List[ControlPoint],
        options: ThreeCPOptions,
        event_manager: EventManager,
        clock: Clock,
    ):
        """Initialize 3CP game with multiple control points."""
        if len(control_points) != 3:
            raise ValueError("ThreeCPGame requires exactly 3 control points")

        self.control_points = control_points
        self.options_3cp = options
        self.events = event_manager
        self.clock = clock

        # Initialize each control point
        for cp in self.control_points:
            cp.init(options.capture_seconds)

        # BaseGame compatibility (use first CP for legacy interface)
        self.control_point = self.control_points[0]

        # Create a minimal GameOptions for BaseGame compatibility
        self.options = GameOptions(
            capture_seconds=options.capture_seconds,
            time_limit_seconds=options.time_limit_seconds,
            start_delay_seconds=options.start_delay_seconds,
        )

        self.reset_game()

    def reset_game(self):
        """Reset game state for new round."""
        print("Resetting game...")
        self._winner = Team.NOBODY
        now = self.clock.milliseconds()
        self._last_update_ms = now
        self._start_time_ms = self.NOT_STARTED
        self.time_added_total = 0

        # Set initial ownership
        self.cp_owners = [CPOwnerState.RED, CPOwnerState.NEUTRAL, CPOwnerState.BLU]
        self.cp_first_capture = [True, False, True]  # A and C already "captured" at start

        # Configure initial point ownership and lock states
        self.control_points[0].set_owner(Team.RED)     # CP-Red: RED owns
        self.control_points[1].set_owner(Team.NOBODY)  # CP-Middle: Neutral
        self.control_points[2].set_owner(Team.BLU)     # CP-Blue: BLU owns

        # Initial lock states: both teams can capture middle
        self._update_capture_locks()

    def _update_capture_locks(self):
        """Update which teams can capture which points based on progression rules."""
        cp_red_owner = self.cp_owners[0]
        cp_center_owner = self.cp_owners[1]
        cp_blue_owner = self.cp_owners[2]
        cp_red = self.control_points[0]
        cp_center = self.control_points[1]
        cp_blue = self.control_points[2]

        # 1. either team can capture the center
        # 2. you can only capture the other teams' point if you own the middle

        cp_center.set_red_capture(True)
        cp_center.set_blu_capture(True)

        if cp_center_owner == CPOwnerState.RED:
            cp_blue.set_red_capture(True)
            cp_blue.set_blu_capture(True)
        else:
            cp_blue.set_red_capture(False)
            cp_blue.set_blu_capture(True)

        if cp_center_owner == CPOwnerState.BLU:
            cp_red.set_red_capture(True)
            cp_red.set_blu_capture(True)
        else:
            cp_red.set_red_capture(True)
            cp_red.set_blu_capture(False)

    def start(self):
        """Start the game."""
        #self.reset_game()
        self._start_time_ms = self.clock.milliseconds()
        if self.events:
            self.events.game_started()

    def end(self):
        """End the game."""
        self._winner = Team.NOBODY
        self._start_time_ms = self.NOT_STARTED
        if self.events:
            self.events.cancelled()

    def is_running(self) -> bool:
        """Check if game is running."""
        return self._start_time_ms != self.NOT_STARTED

    def is_over(self) -> bool:
        """Check if game is over."""
        return not self.is_running() or self._winner != Team.NOBODY

    def get_winner(self) -> Team:
        """Get the winning team."""
        return self._winner

    def get_seconds_elapsed(self) -> int:
        """Get seconds elapsed since game start."""
        if self._start_time_ms == self.NOT_STARTED:
            return 0
        return self.clock.seconds_since(self._start_time_ms)

    def update_control_point(self, cp_index: int, proximity: Proximity):
        """Update a single control point with its proximity data."""
        if 0 <= cp_index < len(self.control_points):
            cp = self.control_points[cp_index]

            # Store previous owner
            prev_owner = cp.get_owner()

            # Update the control point
            cp.update(proximity)

            # Check if ownership changed
            new_owner = cp.get_owner()
            if new_owner != prev_owner and new_owner != Team.NOBODY:
                # Update our tracking
                if new_owner == Team.RED:
                    self.cp_owners[cp_index] = CPOwnerState.RED
                elif new_owner == Team.BLU:
                    self.cp_owners[cp_index] = CPOwnerState.BLU

                # Add time if this is first capture
                if not self.cp_first_capture[cp_index] and self.options_3cp:
                    self.time_added_total += self.options_3cp.add_time_per_first_capture_seconds
                    self.cp_first_capture[cp_index] = True
                    if self.events:
                        mins = self.options_3cp.add_time_per_first_capture_seconds // 60
                        self.events._add_event(f"Time Extended: +{mins} minutes")

                # Update locks after ownership change
                self._update_capture_locks()

    def get_remaining_seconds(self) -> int:
        """Get remaining time in the round."""
        if self._winner != Team.NOBODY:
            return 0

        elapsed = self.get_seconds_elapsed()
        base_limit = self.options_3cp.time_limit_seconds if self.options_3cp else 600
        total_limit = base_limit + self.time_added_total

        remaining = total_limit - elapsed
        return max(0, remaining)

    def check_victory(self) -> Team:
        """
        Check if either team has won.
        Win condition: Own all three control points.
        """
        # Count ownership
        red_count = sum(1 for owner in self.cp_owners if owner == CPOwnerState.RED)
        blu_count = sum(1 for owner in self.cp_owners if owner == CPOwnerState.BLU)

        if red_count == 3:
            return Team.RED
        if blu_count == 3:
            return Team.BLU

        # Check time limit
        if self.get_remaining_seconds() <= 0:
            # Winner is team with more points
            if red_count > blu_count:
                return Team.RED
            elif blu_count > red_count:
                return Team.BLU
            # Tie = no winner (could also be Team.NOBODY for stalemate)

        return Team.NOBODY

    def check_overtime(self) -> bool:
        """
        Overtime when time is up but someone is actively capturing.
        """
        if self.get_remaining_seconds() > 0:
            return False

        # Check if any point is being captured
        for cp in self.control_points:
            if cp.get_capturing() != Team.NOBODY:
                return True

        return False

    def get_cp_state(self, cp_index: int) -> dict:
        """Get state of a specific control point for UI."""
        if cp_index < 0 or cp_index >= len(self.control_points):
            return {}

        cp = self.control_points[cp_index]

        # Get capture multiplier
        from battlepoint_core import Proximity  # keep local import as in your original
        capture_multiplier = 0
        capturing = cp.get_capturing()
        contested = cp.is_contested()

        if not contested and capturing != Team.NOBODY:
            # Would need access to proximity object to get count
            # For now, just indicate if capturing is happening
            capture_multiplier = 1

        return {
            'owner': team_text(cp.get_owner()),
            'capturing': team_text(cp.get_capturing()),
            'on': team_text(cp.get_on()),
            'contested': cp.is_contested(),
            'progress': cp.get_capture_progress_percent(),
            'capture_multiplier': capture_multiplier,
        }


class GamePhase(Enum):
    IDLE = "idle"
    COUNTDOWN = "countdown"
    RUNNING = "running"
    ENDED = "ended"


class ThreeCPBackend:
    """Backend for managing 3CP game state."""

    def __init__(self, sound_system=None, scanner: Optional[EnhancedBLEScanner] = None):
        self.clock = RealClock()
        self.options = ThreeCPOptions()
        self.event_manager = EventManager(self.clock, sound_system)

        # IMPORTANT: allow sharing a single EnhancedBLEScanner instance
        # (e.g., pass in koth_backend.scanner to avoid two scanners).
        self.scanner: EnhancedBLEScanner = scanner if scanner is not None else EnhancedBLEScanner(self.clock)

        print(f" My scanner is: {self.scanner}")

        # Create 3 control points with 3 proximity trackers
        self.control_points = [
            ControlPoint(self.event_manager, self.clock) for _ in range(3)
        ]
        self.proximities = [
            Proximity(
                GameOptions(capture_button_threshold_seconds=self.options.capture_button_threshold_seconds),
                self.clock
            )
            for _ in range(3)
        ]

        self.game: Optional[ThreeCPGame] = None
        self.sound_system = sound_system

        self._phase = GamePhase.IDLE
        self._running = False
        self._countdown_total = 0
        self._countdown_started_ms = 0
        self.game_id: int = 0
        # Manual control per CP
        self.manual_control: bool = False
        self.manual_states: List[Dict[str, bool]] = [
            {'red': False, 'blu': False} for _ in range(3)
        ]

    # ---------- CONFIG / GAME LIFECYCLE ----------

    def configure(self, options: ThreeCPOptions):
        """Configure game options."""
        options.validate()
        self.options = options

        # Update proximity thresholds
        for prox in self.proximities:
            prox.game_options.capture_button_threshold_seconds = options.capture_button_threshold_seconds

    def start_game(self):
        """Start a new game."""

        self.game_id += 1
        self.game = ThreeCPGame()
        self.game.init_3cp(
            self.control_points,
            self.options,
            self.event_manager,
            self.clock,
        )

        self._countdown_total = max(0, int(self.options.start_delay_seconds))
        self._countdown_started_ms = self.clock.milliseconds()

        self.event_manager.starting_game()
        print("Starting Game...")
        self.game.reset_game()
        if self._countdown_total > 0:
            self._phase = GamePhase.COUNTDOWN
            self._running = False
        else:
            self._do_start_game_now()

    #tod-- break this out
    def _ensure_ble(self, scanning: bool):
        async def _go():
            if scanning:
                await self.scanner.start_scanning()
            else:
                await self.scanner.stop_scanning()

        try:
            loop = asyncio.get_running_loop()
            loop.create_task(_go())
        except RuntimeError:
            # No running loop (e.g., called from blocking context) – spin a thread
            threading.Thread(target=lambda: asyncio.run(_go()), daemon=True).start()


    def _do_start_game_now(self):
        """Actually start the game after countdown."""
        if self.game:
            self._ensure_ble(True)
            self.game.start()
            self._phase = GamePhase.RUNNING
            self._running = True

            #if self.sound_system:
            #    self.sound_system.stop()

    def stop_game(self):
        """Stop the current game."""
        if self.game:
            self.game.end()

        self._ensure_ble(False)
        self._phase = GamePhase.IDLE
        self._running = False
        self.game = None

    # ---------- PROXIMITY + UPDATE LOOP ----------

    def _update_proximity(self):
        """Update proximity for all control points based on BLE or manual input."""
        mapping: ControlSquareMapping = self.options.control_square_mapping

        for cp_idx in range(3):

            # BLE mode: count players on squares assigned to this CP
            square_ids = mapping.get_squares_for_cp(cp_idx + 1)
            counts = self.scanner.get_player_counts_for_squares(square_ids)

            current_owner = self.control_points[cp_idx].get_owner()
            red_on = counts.get('red',0)
            blue_on = counts.get('blu',0)
            mag_on = counts.get('mag',0)

            #adjust for manual control
            if self.manual_control:
                # Manual mode: allow operator to simulate 1 user on
                if self.manual_states[cp_idx]['red']:
                    red_on += 1
                if self.manual_states[cp_idx]['blu']:
                    blue_on += 1

            #we dont know the color of a player with magnetic presence.
            #BUT we can assume that if the current owner is one team,
            # a magnetic player is the OTHER team. why would a team stand on a point it owns?
            if current_owner == Team.RED:
                blue_on += mag_on
            elif current_owner == Team.BLU:
                red_on += mag_on

            self.proximities[cp_idx].update_counts(red_on, blue_on)

    def update(self):
        """Main update loop."""
        now_ms = self.clock.milliseconds()

        # Handle countdown
        if self._phase == GamePhase.COUNTDOWN:
            elapsed_ms = now_ms - self._countdown_started_ms
            if elapsed_ms < 0:
                elapsed_ms = 0
                self._countdown_started_ms = now_ms

            elapsed_s = elapsed_ms // 1000
            remaining = max(0, self._countdown_total - elapsed_s)

            self.event_manager.starts_in_seconds(remaining)

            if remaining <= 0:
                self._do_start_game_now()
            return

        # If not running, do nothing
        if self._phase != GamePhase.RUNNING or not self.game:
            return

        # Update proximity for all CPs
        try:
            self._update_proximity()
        except Exception:
            traceback.print_exc()

        # Update all control points
        for i in range(3):
            self.game.update_control_point(i, self.proximities[i])

        # Check victory/overtime
        winner = self.game.check_victory()
        if winner != Team.NOBODY:
            self.game._winner = winner
            self._phase = GamePhase.ENDED
            self._running = False
            if self.event_manager:
                self.event_manager.victory(winner)
            return

        if self.game.check_overtime():
            if self.event_manager:
                self.event_manager.overtime()

        # Countdown announcements
        if self.event_manager:
            self.event_manager.ends_in_seconds(self.game.get_remaining_seconds())

    # ---------- STATE / MANUAL CONTROL ----------

    def get_state(self) -> dict:
        """Get current game state for API."""
        events = [ev.to_display() for ev in self.event_manager.get_events(100)]

        # Countdown phase
       # if self._phase == GamePhase.COUNTDOWN:
       #     now_ms = self.clock.milliseconds()
       #     elapsed_ms = now_ms - self._countdown_started_ms
       #     elapsed_s = elapsed_ms // 1000
       #     remaining = max(0, self._countdown_total - elapsed_s)
       #     return {
       #         'running': False,
       #         'phase': 'countdown',
       #         'countdown_remaining': remaining,
       #
       #         'events': events,
       #        'game_id': self.game_id,
       #     }

        # Running or ended
        if self.game and self._phase in (GamePhase.COUNTDOWN, GamePhase.RUNNING, GamePhase.ENDED):


            def compute_phase_text():
                if self._phase == GamePhase.COUNTDOWN:
                    return 'countdown'
                elif self._phase == GamePhase.RUNNING:
                    return 'running'
                elif self._phase == GamePhase.ENDED:
                    return 'ended'
                else:
                    return str(self._phase)

            cp_states = [self.game.get_cp_state(i) for i in range(3)]

            remaining=0
            if self._phase == GamePhase.COUNTDOWN:
                now_ms = self.clock.milliseconds()
                elapsed_ms = now_ms - self._countdown_started_ms
                elapsed_s = elapsed_ms // 1000
                remaining = max(0, self._countdown_total - elapsed_s)


            return {
                'running': False if self._phase == self._running else self._running,
                'phase': compute_phase_text(),
                'countdown_remaining': remaining,
                'remaining_seconds': self.game.get_remaining_seconds(),
                'time_limit_seconds': self.options.time_limit_seconds,
                'time_added_seconds': self.game.time_added_total,
                'winner': team_text(self.game.get_winner()) if self.game.is_over() else None,
                'control_points': cp_states,
                'cp_owners': [owner.name for owner in self.game.cp_owners],
                'events': events,
                'game_id': self.game_id,
            }

        # Idle
        return {
            'running': False,
            'phase': 'idle',
            'events': events,
            'game_id': self.game_id,
        }

    # Manual control API
    def set_manual_control(self, enabled: bool):
        self.manual_control = bool(enabled)
        if not self.manual_control:
            for state in self.manual_states:
                state['red'] = False
                state['blu'] = False

    def set_manual_state(self, cp_index: int, red: Optional[bool] = None, blu: Optional[bool] = None):
        if 0 <= cp_index < 3:
            if red is not None:
                self.manual_states[cp_index]['red'] = bool(red)
            if blu is not None:
                self.manual_states[cp_index]['blu'] = bool(blu)

    def get_manual_state(self) -> dict:
        return {
            "manual_control": self.manual_control,
            "cp_states": self.manual_states,
        }

    # -------- BLE DEBUG SUMMARY FOR /api/bluetooth/devices --------

    def get_ble_devices_summary(self) -> dict:
        """
        Summary helper for the debug endpoint.

        Uses whatever scanner instance this backend is holding. Assumes the
        scanner object tracks a `scanning` flag and either:
          - provides get_devices_snapshot() -> (devices, scanning), or
          - exposes .devices dict and .scanning bool.
        """
        scanner = getattr(self, "scanner", None)
        if scanner is None:
            return {"scanning": False, "device_count": 0, "devices": []}

        try:
            if hasattr(scanner, "get_devices_snapshot"):
                devices, scanning = scanner.get_devices_snapshot()
            else:
                devices_dict = getattr(scanner, "devices", {})
                if isinstance(devices_dict, dict):
                    devices = list(devices_dict.values())
                else:
                    devices = []
                scanning = bool(getattr(scanner, "scanning", False))

            return {
                "scanning": scanning,
                "device_count": len(devices),
                "devices": devices,
            }
        except Exception as e:
            print(f"[DEBUG] ThreeCPBackend.get_ble_devices_summary error: {e}")
            return {"scanning": False, "device_count": 0, "devices": []}
