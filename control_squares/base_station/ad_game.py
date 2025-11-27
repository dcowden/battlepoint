"""
AD (Attack / Defend) Game Mode
BLUE attacks, RED defends.

- Up to 3 control points (CP1, CP2, CP3)
- Which CPs are active is determined by control_square_mapping:
  - If a CP has no squares assigned, it is not used.
  - User must assign CPs in order starting from CP1.
- All *used* control points start owned by RED.
- Control points cannot be recaptured by RED once BLUE captures them.
- BLUE must capture the used points in order (1 -> 2 -> 3) within the time limit.
- On first capture of each used CP, extra time is added to the game.
- Magnetic presence is treated as BLUE presence (attacker) in proximity logic.
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
    """Ownership state for a control point in AD."""
    RED = 1
    BLU = 2
    NEUTRAL = 3


class ADGame(BaseGame):
    """
    AD game mode with linear progression.

    Layout conceptually: [1] -> [2] -> [3]
    - Only the CPs that have squares assigned are "used".
    - All used CPs start owned by RED (defenders).
    - BLUE must capture used CPs in order (1 -> 2 -> 3).
    - Control points cannot be recaptured by RED.
    - Time is added once per CP when BLUE captures it for the first time.
    """

    def __init__(self):
        super().__init__()
        self.control_points: List[ControlPoint] = []
        self.cp_owners: List[CPOwnerState] = [CPOwnerState.NEUTRAL] * 3
        self.cp_captured_by_blue: List[bool] = [False, False, False]
        self.cp_used: List[bool] = [False, False, False]  # which CPs are active
        self.time_added_total: int = 0
        self.options_ad: Optional[ThreeCPOptions] = None

    def init_ad(
        self,
        control_points: List[ControlPoint],
        options: ThreeCPOptions,
        event_manager: EventManager,
        clock: Clock,
        cp_used: List[bool],
    ):
        """Initialize AD game with up to 3 control points."""
        if len(control_points) != 3:
            raise ValueError("ADGame requires exactly 3 control points")

        if len(cp_used) != 3:
            raise ValueError("ADGame cp_used must be length 3")

        self.control_points = control_points
        self.cp_used = cp_used[:]
        self.options_ad = options
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
        print("Resetting AD game...")
        self._winner = Team.NOBODY
        now = self.clock.milliseconds()
        self._last_update_ms = now
        self._start_time_ms = self.NOT_STARTED
        self.time_added_total = 0

        # Initial ownership:
        # - Used CPs start RED
        # - Unused CPs treated as NEUTRAL and disabled
        self.cp_owners = []
        self.cp_captured_by_blue = []
        for idx, cp in enumerate(self.control_points):
            if self.cp_used[idx]:
                self.cp_owners.append(CPOwnerState.RED)
                self.cp_captured_by_blue.append(False)
                cp.set_owner(Team.RED)
            else:
                self.cp_owners.append(CPOwnerState.NEUTRAL)
                self.cp_captured_by_blue.append(False)
                cp.set_owner(Team.NOBODY)

        self._update_capture_locks()

    def _blue_can_capture_cp(self, cp_index: int) -> bool:
        """
        BLUE can capture CP i only if all *used* earlier CPs (0..i-1)
        have already been captured by BLUE.
        """
        if not self.cp_used[cp_index]:
            return False

        for j in range(cp_index):
            if self.cp_used[j] and self.cp_owners[j] != CPOwnerState.BLU:
                return False
        return True

    def _update_capture_locks(self):
        """
        Update which teams can capture which points based on progression rules.

        - RED should never capture in AD (no recapture), so set_red_capture(False).
        - BLUE capture is allowed only when linear progression condition is satisfied.
        """
        for cp_index, cp in enumerate(self.control_points):
            if not self.cp_used[cp_index]:
                # Unused: no one can capture
                cp.set_red_capture(False)
                cp.set_blu_capture(False)
                continue

            cp.set_red_capture(False)  # RED cannot (re)capture in AD

            if self._blue_can_capture_cp(cp_index):
                cp.set_blu_capture(True)
            else:
                cp.set_blu_capture(False)

    def start(self):
        """Start the game."""
        # self.reset_game() is called by backend at start_game, then we start here.
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
        if 0 > cp_index or cp_index >= len(self.control_points):
            return
        if not self.cp_used[cp_index]:
            return

        cp = self.control_points[cp_index]

        # Store previous owner
        prev_owner = cp.get_owner()

        # Update the control point
        cp.update(proximity)

        # Check if ownership changed
        new_owner = cp.get_owner()

        # Only care about BLUE capturing from RED in this mode
        if new_owner != prev_owner and new_owner == Team.BLU:
            # Update our tracking
            self.cp_owners[cp_index] = CPOwnerState.BLU

            # Add time on first BLUE capture for this CP
            if not self.cp_captured_by_blue[cp_index] and self.options_ad:
                add_secs = self.options_ad.add_time_per_first_capture_seconds
                if add_secs > 0:
                    self.time_added_total += add_secs
                    self.cp_captured_by_blue[cp_index] = True
                    if self.events:
                        mins = add_secs // 60
                        if mins > 0:
                            self.events._add_event(f"Time Extended: +{mins} minutes")
                        else:
                            self.events._add_event(f"Time Extended: +{add_secs} seconds")

            # Update locks after ownership change
            self._update_capture_locks()

    def get_remaining_seconds(self) -> int:
        """Get remaining time in the round (including any added time)."""
        if self._winner != Team.NOBODY:
            return 0

        elapsed = self.get_seconds_elapsed()
        base_limit = self.options_ad.time_limit_seconds if self.options_ad else 600
        total_limit = base_limit + self.time_added_total

        remaining = total_limit - elapsed
        return max(0, remaining)

    def check_victory(self) -> Team:
        """
        Check if either team has won.

        Win conditions:
        - BLUE wins if all used CPs are owned by BLUE.
        - RED wins if time runs out and BLUE has not captured all used CPs.
        """
        active_indices = [i for i, used in enumerate(self.cp_used) if used]
        if not active_indices:
            # No CPs configured -> no winner (treat as invalid game config)
            return Team.NOBODY

        blue_count = sum(
            1 for i in active_indices if self.cp_owners[i] == CPOwnerState.BLU
        )
        total_active = len(active_indices)

        # BLUE captured all active CPs
        if blue_count == total_active:
            return Team.BLU

        # Check time limit
        if self.get_remaining_seconds() <= 0:
            # Time is up and BLUE has not captured all points -> RED wins
            return Team.RED

        return Team.NOBODY

    def check_overtime(self) -> bool:
        """
        Overtime when time is up but BLUE is actively capturing any used point.
        """
        if self.get_remaining_seconds() > 0:
            return False

        for idx, cp in enumerate(self.control_points):
            if not self.cp_used[idx]:
                continue
            if cp.get_capturing() == Team.BLU:
                return True

        return False

    def get_cp_state(self, cp_index: int) -> dict:
        """Get state of a specific control point for UI."""
        if cp_index < 0 or cp_index >= len(self.control_points):
            return {}

        cp = self.control_points[cp_index]

        # Get capture multiplier (same simple logic as ThreeCPGame)
        capture_multiplier = 0
        capturing = cp.get_capturing()
        contested = cp.is_contested()

        if not contested and capturing != Team.NOBODY:
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


class ADBackend:
    """Backend for managing AD game state."""

    def __init__(self, sound_system=None, scanner: Optional[EnhancedBLEScanner] = None):
        self.clock = RealClock()
        self.options = ThreeCPOptions()
        self.event_manager = EventManager(self.clock, sound_system)

        # Allow sharing a single EnhancedBLEScanner instance
        self.scanner: EnhancedBLEScanner = scanner if scanner is not None else EnhancedBLEScanner(self.clock)

        print(f"[ADBackend] My scanner is: {self.scanner}")

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

        self.game: Optional[ADGame] = None
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

        # Track which CPs are active (have any squares assigned)
        self.cp_used: List[bool] = [True, False, False]

    # ---------- CONFIG / GAME LIFECYCLE ----------

    def configure(self, options: ThreeCPOptions):
        """Configure game options."""
        options.validate()
        self.options = options

        # Determine which CPs are "used" based on control_square_mapping
        mapping: ControlSquareMapping = self.options.control_square_mapping
        self.cp_used = [
            bool(mapping.get_squares_for_cp(1)),
            bool(mapping.get_squares_for_cp(2)),
            bool(mapping.get_squares_for_cp(3)),
        ]
        print(f"[ADBackend] configure: cp_used = {self.cp_used}")

        # Update proximity thresholds
        for prox in self.proximities:
            prox.game_options.capture_button_threshold_seconds = options.capture_button_threshold_seconds

    def start_game(self):
        """Start a new AD game."""
        # Ensure we have at least one active CP
        if not any(self.cp_used):
            print("[ADBackend] WARNING: start_game called with no active control points.")
            # We'll still start the game, but it will never end via captures
            # and will end only on time limit as Team.RED.
        self.game_id += 1
        self.game = ADGame()
        self.game.init_ad(
            self.control_points,
            self.options,
            self.event_manager,
            self.clock,
            self.cp_used,
        )

        self._countdown_total = max(0, int(self.options.start_delay_seconds))
        self._countdown_started_ms = self.clock.milliseconds()

        self.event_manager.starting_game()
        print("[ADBackend] Starting AD Game...")

        self.game.reset_game()
        if self._countdown_total > 0:
            self._phase = GamePhase.COUNTDOWN
            self._running = False
        else:
            self._do_start_game_now()

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
            threading.Thread(target=lambda: asyncio.run(_go()), daemon=True).start()

    def _do_start_game_now(self):
        """Actually start the game after countdown."""
        if self.game:
            self._ensure_ble(True)
            self.game.start()
            self._phase = GamePhase.RUNNING
            self._running = True

            if self.sound_system:
                self.sound_system.stop()

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
        """
        Update proximity for all control points based on BLE or manual input.

        AD-specific rule:
        - Magnetic presence is treated as BLUE presence, since points cannot be recaptured and BLUE is always the attacker.
        """
        mapping: ControlSquareMapping = self.options.control_square_mapping

        for cp_idx in range(3):
            square_ids = mapping.get_squares_for_cp(cp_idx + 1)
            counts = self.scanner.get_player_counts_for_squares(square_ids)
            current_owner = self.control_points[cp_idx].get_owner()
            red_on = counts.get('red', 0)
            blue_on = counts.get('blu', 0)
            mag_on = counts.get('mag', 0)

            # Manual mode: allow operator to simulate 1 user on
            if self.manual_control:
                if self.manual_states[cp_idx]['red']:
                    red_on += 1
                if self.manual_states[cp_idx]['blu']:
                    blue_on += 1

            # AD rule: magnetic presence counts as BLUE (attacker)
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

        # Running / ended / countdown
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

            # Build full CP state, then filter to used CPs for UI
            cp_states_all = [self.game.get_cp_state(i) for i in range(3)]
            cp_states = [
                cp_states_all[i]
                for i in range(3)
                if self.game.cp_used[i]
            ]

            countdown_remaining = 0
            if self._phase == GamePhase.COUNTDOWN:
                now_ms = self.clock.milliseconds()
                elapsed_ms = now_ms - self._countdown_started_ms
                elapsed_s = elapsed_ms // 1000
                countdown_remaining = max(0, self._countdown_total - elapsed_s)

            return {
                'running': False if self._phase == self._running else self._running,
                'phase': compute_phase_text(),
                'countdown_remaining': countdown_remaining,
                'elapsed_seconds': self.game.get_seconds_elapsed(),
                'remaining_seconds': self.game.get_remaining_seconds(),
                'time_limit_seconds': self.options.time_limit_seconds,
                'time_added_seconds': self.game.time_added_total,
                'winner': team_text(self.game.get_winner()) if self.game.is_over() else None,
                'control_points': cp_states,
                'cp_owners': [
                    owner.name
                    for i, owner in enumerate(self.game.cp_owners)
                    if self.game.cp_used[i]
                ],
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
        Summary helper for debug endpoint.

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
            print(f"[DEBUG] ADBackend.get_ble_devices_summary error: {e}")
            return {"scanning": False, "device_count": 0, "devices": []}
