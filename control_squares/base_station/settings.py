"""
Settings and configuration for BattlePoint game modes.
Handles both KOTH and 3CP game settings.
"""

from dataclasses import dataclass, field, asdict
from typing import Optional, Dict, List
import json
import os
from battlepoint_core import GameMode, GameOptions


@dataclass
class ControlSquareMapping:
    """Maps control squares (CS-XX devices) to control points."""
    cp_1_squares: List[int] = field(default_factory=lambda: [1])  # e.g., [1, 2] for CS-01, CS-02
    cp_2_squares: List[int] = field(default_factory=lambda: [3])  # e.g., [3] for CS-03
    cp_3_squares: List[int] = field(default_factory=lambda: [5])  # e.g., [5] for CS-05

    def get_cp_for_square(self, square_id: int) -> Optional[int]:
        """Returns which CP (1, 2, or 3) owns this square, or None."""
        if square_id in self.cp_1_squares:
            return 1
        if square_id in self.cp_2_squares:
            return 2
        if square_id in self.cp_3_squares:
            return 3
        return None

    def get_squares_for_cp(self, cp_num: int) -> List[int]:
        """Returns list of square IDs for a given CP."""
        if cp_num == 1:
            return self.cp_1_squares
        elif cp_num == 2:
            return self.cp_2_squares
        elif cp_num == 3:
            return self.cp_3_squares
        return []

    def to_dict(self) -> dict:
        return {
            'cp_1_squares': self.cp_1_squares,
            'cp_2_squares': self.cp_2_squares,
            'cp_3_squares': self.cp_3_squares,
        }

    @staticmethod
    def from_dict(data: dict) -> 'ControlSquareMapping':
        return ControlSquareMapping(
            cp_1_squares=data.get('cp_1_squares', [1]),
            cp_2_squares=data.get('cp_2_squares', [3]),
            cp_3_squares=data.get('cp_3_squares', [5]),
        )


@dataclass
class ThreeCPOptions:
    """Settings specific to 3CP game mode."""
    capture_seconds: int = 20
    capture_button_threshold_seconds: int = 5
    time_limit_seconds: int = 600  # 10 minutes default
    start_delay_seconds: int = 5
    add_time_per_first_capture_seconds: int = 120  # Add 2 minutes when a CP is captured for first time

    # Control square mappings
    control_square_mapping: ControlSquareMapping = field(default_factory=ControlSquareMapping)

    def validate(self):
        """Validate settings."""
        if self.capture_seconds >= self.time_limit_seconds:
            self.capture_seconds = self.time_limit_seconds - 1
        if self.capture_button_threshold_seconds >= self.capture_seconds:
            self.capture_button_threshold_seconds = self.capture_seconds - 1

    def to_dict(self) -> dict:
        return {
            'capture_seconds': self.capture_seconds,
            'capture_button_threshold_seconds': self.capture_button_threshold_seconds,
            'time_limit_seconds': self.time_limit_seconds,
            'start_delay_seconds': self.start_delay_seconds,
            'add_time_per_first_capture_seconds': self.add_time_per_first_capture_seconds,
            'control_square_mapping': self.control_square_mapping.to_dict(),
        }

    @staticmethod
    def from_dict(data: dict) -> 'ThreeCPOptions':
        mapping_data = data.get('control_square_mapping', {})
        return ThreeCPOptions(
            capture_seconds=data.get('capture_seconds', 20),
            capture_button_threshold_seconds=data.get('capture_button_threshold_seconds', 5),
            time_limit_seconds=data.get('time_limit_seconds', 600),
            start_delay_seconds=data.get('start_delay_seconds', 5),
            add_time_per_first_capture_seconds=data.get('add_time_per_first_capture_seconds', 120),
            control_square_mapping=ControlSquareMapping.from_dict(mapping_data),
        )


class UnifiedSettingsManager:
    """Manages settings for both KOTH and 3CP game modes."""

    def __init__(self, koth_file: str = "koth_settings.json", threecp_file: str = "3cp_settings.json"):
        self.koth_file = koth_file
        self.threecp_file = threecp_file

    # ========== KOTH Settings ==========

    def save_koth_settings(self, options: GameOptions, volume: int = 10, brightness: int = 50) -> bool:
        try:
            data = {
                'mode': options.mode.value,
                'capture_seconds': options.capture_seconds,
                'capture_button_threshold_seconds': options.capture_button_threshold_seconds,
                'time_limit_seconds': options.time_limit_seconds,
                'start_delay_seconds': options.start_delay_seconds,
                'volume': volume,
                'brightness': brightness,
            }
            with open(self.koth_file, 'w') as f:
                json.dump(data, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving KOTH settings: {e}")
            return False

    def load_koth_settings(self) -> Optional[tuple[GameOptions, int, int]]:
        if not os.path.exists(self.koth_file):
            return None
        try:
            with open(self.koth_file, 'r') as f:
                data = json.load(f)

            options = GameOptions(
                mode=GameMode(data.get('mode', 0)),
                capture_seconds=data.get('capture_seconds', 20),
                capture_button_threshold_seconds=data.get('capture_button_threshold_seconds', 5),
                time_limit_seconds=data.get('time_limit_seconds', 60),
                start_delay_seconds=data.get('start_delay_seconds', 5),
            )
            volume = data.get('volume', 10)
            brightness = data.get('brightness', 50)
            return options, volume, brightness
        except Exception as e:
            print(f"Error loading KOTH settings: {e}")
            return None

    # ========== 3CP Settings ==========

    def save_3cp_settings(self, options: ThreeCPOptions, volume: int = 10, brightness: int = 50) -> bool:
        try:
            data = options.to_dict()
            data['volume'] = volume
            data['brightness'] = brightness
            print('Saving 3CP settings to', self.threecp_file, 'data=', data)  # optional debug

            with open(self.threecp_file, 'w') as f:
                json.dump(data, f, indent=2)
            return True
        except Exception as e:
            print(f"Error saving 3CP settings: {e}")
            return False


    def load_3cp_settings(self) -> Optional[tuple[ThreeCPOptions, int, int]]:
        if not os.path.exists(self.threecp_file):
            return None
        try:
            with open(self.threecp_file, 'r') as f:
                data = json.load(f)

            options = ThreeCPOptions.from_dict(data)
            volume = data.get('volume', 10)
            brightness = data.get('brightness', 50)
            return options, volume, brightness
        except Exception as e:
            print(f"Error loading 3CP settings: {e}")
            return None