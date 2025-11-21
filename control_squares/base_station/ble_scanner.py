import traceback
from typing import Optional, List, Dict
import asyncio
import sys
import threading
import time
import re, subprocess

from battlepoint_core import (
    Clock,
    BluetoothTag,
    Team,
    TagType,
)

MAX_PLAYERS_PER_TEAM = 3

# Linux: use bleson
if sys.platform != "win32":
    from bleson import get_provider, Observer
else:
    get_provider = None
    Observer = None

# Windows: keep Bleak
try:
    from bleak import BleakScanner
except ImportError:
    BleakScanner = None


def pick_usb_hci_index():
    """
    Parse `hciconfig -a` output and return the index of the adapter
    whose Bus is USB. Handles 'Bus: USB' on the same line as hciX:,
    or on the following line. Falls back to 0 if none found.
    """
    try:
        out = subprocess.check_output(
            ["hciconfig", "-a"], text=True, stderr=subprocess.STDOUT
        )
    except Exception as e:
        print(f"[ERROR] Failed to run hciconfig: {e}")
        print("[INFO] Falling back to hci0")
        return 0

    lines = out.splitlines()
    current_idx = None
    current_bus = None
    usb_indices = []

    for i, line in enumerate(lines):
        line = line.rstrip()

        # Match hci header line: "hci0:   Type: Primary  Bus: USB"
        m = re.match(r"^hci(\d+):", line)
        if m:
            # flush previous adapter
            if current_idx is not None and current_bus == "USB":
                usb_indices.append(current_idx)

            current_idx = int(m.group(1))
            current_bus = None

            # Check for Bus: on the same line
            if "Bus:" in line:
                parts = line.split("Bus:", 1)
                bus = parts[1].strip().split()[0]
                current_bus = bus
            continue

        # If Bus: is on a following line, catch it here
        if "Bus:" in line and current_idx is not None and current_bus is None:
            parts = line.split("Bus:", 1)
            bus = parts[1].strip().split()[0]
            current_bus = bus

    # Flush last adapter block
    if current_idx is not None and current_bus == "USB":
        usb_indices.append(current_idx)

    if usb_indices:
        chosen = usb_indices[0]
        print(f"[INFO] Detected USB Bluetooth adapter: hci{chosen}")
        return chosen

    print("[WARN] No USB adapter detected via hciconfig, falling back to hci0")
    return 0


# ============================================================================
# CONFIG / CONSTANTS
# ============================================================================

_TILE_PREFIX = "CS-"
_TEAM_CHARS = ("B", "R")


class EnhancedBLEScanner:
    _CS_PREFIX = "CS-"
    _PLAYER_PREFIX = "PT-"
    FRESH_WINDOW_MS = 1000  # slightly generous; adjust if you want tighter

    def __init__(self, clock: Clock, linux_adapter_index: int = 1):
        """
        linux_adapter_index: HCI index for bleson (0 -> hci0, 1 -> hci1, etc.).
        """
        self.clock = clock
        self.devices: Dict[str, dict] = {}
        self.fresh_window_ms = EnhancedBLEScanner.FRESH_WINDOW_MS

        # Common state
        self._lock = threading.Lock()
        self._scanning: bool = False

        # Platform split
        self._is_windows = (sys.platform == "win32")

        # ---------------- Windows (Bleak) bits ----------------
        self._scanner: Optional["BleakScanner"] = None
        self._want_scan: bool = False
        self._thread: Optional[threading.Thread] = None
        self._thread_loop: Optional[asyncio.AbstractEventLoop] = None

        # ---------------- Linux (Bleson) bits ----------------
        self._linux_adapter_index = pick_usb_hci_index()
        self._linux_adapter = None
        self._linux_observer: Optional["Observer"] = None

        # DEBUG: track last callback time per address for gap measurement
        self._last_cb_time_by_addr: Dict[str, int] = {}
        self._cb_count_by_addr: Dict[str, int] = {}

        if self._is_windows:
            self._start_windows_thread()

    # ======================================================================
    # WINDOWS PATH (Bleak)
    # ======================================================================

    def _start_windows_thread(self):
        if self._thread is not None:
            return

        if BleakScanner is None:
            print("[BLE] Bleak not available on Windows; scanner disabled.")
            return

        def _runner():
            asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            self._thread_loop = loop
            loop.run_until_complete(self._windows_scanner_main())

        self._thread = threading.Thread(target=_runner, daemon=True)
        self._thread.start()
        print("[BLE] Windows scanner thread started")

    async def _windows_scanner_main(self):
        self._scanner = BleakScanner(detection_callback=self._bleak_callback)

        while True:
            try:
                if self._want_scan and not self._scanning:
                    try:
                        await self._scanner.start()
                        self._scanning = True
                        print("[BLE] (win) started scanning")
                    except Exception as e:
                        print(f"[BLE] (win) error starting scan: {e!r}")
                        self._scanning = False
                        await asyncio.sleep(1.0)

                elif not self._want_scan and self._scanning:
                    try:
                        await self._scanner.stop()
                    except Exception as e:
                        print(f"[BLE] (win) error stopping scan: {e!r}")
                    self._scanning = False
                    print("[BLE] (win) stopped scanning")

            except Exception as e:
                print(f"[BLE] (win) main loop error: {e!r}")

            await asyncio.sleep(0.2)

    def _bleak_callback(self, device, advertisement_data):
        """
        Windows path: Bleak gives us a single advertisement at a time.
        We extract MFG ASCII and apply the same tile logic as Linux.
        """
        name = device.name or ""
        address = device.address.lower()
        rssi = advertisement_data.rssi
        current_time = self.clock.milliseconds()

        mfg_ascii = ""
        if advertisement_data.manufacturer_data:
            for _, data_bytes in advertisement_data.manufacturer_data.items():
                try:
                    mfg_ascii = data_bytes.decode("ascii", errors="ignore")
                except Exception:
                    mfg_ascii = data_bytes.hex()
                break

        tile_id, logical_name = self._extract_tile_from_mfg(mfg_ascii)
        if tile_id is None:
            # Not one of ours
            return

        self._record_observation(
            address=address,
            logical_name=logical_name,
            rssi=rssi,
            mfg_ascii=mfg_ascii,
            now_ms=current_time,
        )

    # ======================================================================
    # LINUX PATH (Bleson)
    # ======================================================================

    def _format_mac_from_bleson(self, addr_obj) -> str:
        """
        bleson BDAddress prints as: BDAddress('AA:BB:CC:DD:EE:FF').
        Normalize to lowercase 'aa:bb:cc:dd:ee:ff'.
        """
        s = str(addr_obj)
        if s.startswith("BDAddress('") and s.endswith("')"):
            s = s[11:-2]
        return s.lower()

    def _decode_mfg_from_bleson(self, mfg_bytes: Optional[bytes]) -> str:
        """Return ASCII manufacturer payload if possible, else empty string."""
        if mfg_bytes is None:
            return ""
        try:
            return mfg_bytes.decode("ascii", errors="ignore")
        except Exception:
            return ""

    def _bleson_callback(self, advertisement):
        """
        Linux path: this is called by bleson for each advertisement.

        We:
          - normalize MAC
          - decode manufacturer data to ASCII
          - use manufacturer data only to decide if it's ours
          - record observation
        """
        now_ms = self.clock.milliseconds()

        address = self._format_mac_from_bleson(advertisement.address)
        rssi = advertisement.rssi
        mfg_ascii = self._decode_mfg_from_bleson(getattr(advertisement, "mfg_data", None))

        tile_id, logical_name = self._extract_tile_from_mfg(mfg_ascii)
        if tile_id is None:
            return  # not our tile

        self._record_observation(
            address=address,
            logical_name=logical_name,
            rssi=rssi,
            mfg_ascii=mfg_ascii,
            now_ms=now_ms,
        )

    # ======================================================================
    # SHARED RECORD / SUMMARY
    # ======================================================================

    def _extract_tile_from_mfg(self, mfg_ascii: str) -> (Optional[str], Optional[str]):
        if not mfg_ascii:
            return None, None

        s = mfg_ascii.strip()
        tile_id = None

        # Format 2: "B,CS-02,PT-...,..." etc.
        if tile_id is None:
            parts = s.split(",")
            if len(parts) >= 2:
                maybe_tile = parts[0].strip().upper()
                if maybe_tile.startswith(_TILE_PREFIX):
                    tile_id = maybe_tile

        if tile_id is None:
            return None, None

        logical_name = tile_id  # what we show as "name"
        return tile_id, logical_name

    def _record_observation(
        self,
        address: str,
        logical_name: str,
        rssi: int,
        mfg_ascii: str,
        now_ms: int,
    ):
        """
        Update self.devices for a single adv from a specific MAC.
        Also logs gaps and cb_count for debugging.
        """
        prev = self._last_cb_time_by_addr.get(address)
        cb_count = self._cb_count_by_addr.get(address, 0) + 1
        self._cb_count_by_addr[address] = cb_count

        if prev is not None:
            gap = now_ms - prev
            print(
                f"[BLE] cb addr={address} name={logical_name} "
                f"gap={gap:4d}ms rssi={rssi:4d} mf='{mfg_ascii}'"
            )
        else:
            print(
                f"[BLE] first cb addr={address} name={logical_name} "
                f"rssi={rssi:4d} mf='{mfg_ascii}'"
            )

        self._last_cb_time_by_addr[address] = now_ms

        with self._lock:
            self.devices[address] = {
                "name": logical_name,
                "rssi": rssi,
                "address": address,
                "manufacturer_data": mfg_ascii,
                "last_seen": now_ms,
                "cb_count": cb_count,
            }

    # ======================================================================
    # PUBLIC API: start/stop
    # ======================================================================

    async def start_scanning(self):
        """
        On Windows: tell the Bleak thread to start.
        On Linux:   open adapter and start a bleson.Observer.
        """
        if self._is_windows:
            self._want_scan = True
            return

        # Linux / bleson
        if self._scanning:
            return

        if get_provider is None or Observer is None:
            print("[BLE] bleson not available; Linux scanning disabled.")
            return

        provider = get_provider()
        adapter = provider.get_adapter(self._linux_adapter_index)
        adapter.open()

        observer = Observer(adapter)
        observer.on_advertising_data = self._bleson_callback
        observer.start()

        self._linux_adapter = adapter
        self._linux_observer = observer
        self._scanning = True
        print(f"[BLE] (linux/bleson) scanning started on hci{self._linux_adapter_index}")

    async def stop_scanning(self):
        if self._is_windows:
            self._want_scan = False
            return

        # Linux / bleson
        if not self._scanning:
            return

        try:
            if self._linux_observer is not None:
                self._linux_observer.stop()
        except Exception as e:
            print(f"[BLE] (linux/bleson) error stopping observer: {e!r}")

        try:
            if self._linux_adapter is not None:
                self._linux_adapter.close()
        except Exception as e:
            print(f"[BLE] (linux/bleson) error closing adapter: {e!r}")

        self._linux_observer = None
        self._linux_adapter = None
        self._scanning = False
        print(f"[BLE] (linux/bleson) scanning stopped on hci{self._linux_adapter_index}")

    # ======================================================================
    # READ METHODS / GAME INTEGRATION
    # ======================================================================

    def get_devices_summary(self) -> dict:
        with self._lock:
            current_time = self.clock.milliseconds()
            device_list = []
            for address, info in self.devices.items():
                age = int(current_time - info["last_seen"])
                device_list.append(
                    {
                        "name": info["name"],
                        "rssi": info["rssi"],
                        "address": address,
                        "last_seen_ms": age,
                        "manufacturer_data": info["manufacturer_data"],
                        "cb_count": info.get("cb_count", 0),
                        "last_gap_ms": (
                            (current_time - self._last_cb_time_by_addr[address])
                            if address in self._last_cb_time_by_addr
                            else None
                        ),
                    }
                )

        device_list.sort(key=lambda d: d["rssi"], reverse=True)

        scanning_flag = self._scanning if not self._is_windows else self._want_scan

        return {
            "scanning": scanning_flag,
            "device_count": len(device_list),
            "devices": device_list,
        }

    def get_active_tags(self, tag_type=None) -> List[BluetoothTag]:
        # Kept for compatibility; currently unused.
        return []

    # ---------- Helper parsing for team ----------

    def get_player_color(self, manufacturer_data: str) -> Optional[str]:
        """
        Parse manufacturer data from a CS-0X device.

        Expected format:
            (B|R),CS-NN,PT-XXX,1234

        Returns:
            'B' or 'R' if valid, otherwise None.
        """
        if not manufacturer_data:
            return None

        parts = manufacturer_data.split(",")
        if not parts:
            return None

        team = parts[1].strip().upper()
        if team in ("B", "R","M","T"):
            return team

        return None

    def _is_control_square(self, info: dict) -> bool:
        """
        Decide if this device should count as a tile.
        """
        name = (info.get("name") or "").strip()
        if name.startswith(self._CS_PREFIX):
            return True

        mf = info.get("manufacturer_data") or ""
        if not mf:
            return False

        parts = mf.split(",")
        if len(parts) >= 2:
            tile_id = parts[1].strip().upper()
            if tile_id.startswith(self._CS_PREFIX):
                info["name"] = tile_id
                return True

        return False

    # ---------- Player count logic ----------

    def get_player_counts(self) -> dict:
        red = 0
        blu = 0
        now = self.clock.milliseconds()
        fresh_ms = self.fresh_window_ms

        with self._lock:
            for _, info in self.devices.items():
                if not self._is_control_square(info):
                    print(f"Warning: skipping {info['name']} ", file=sys.stderr)
                    continue

                last_seen = info.get("last_seen", 0.0) or 0.0
                age = now - last_seen
                if age > fresh_ms:
                    continue

                mf = info.get("manufacturer_data") or ""
                team_letter = self.get_player_color(mf)
                if team_letter == "B":
                    blu += 1
                elif team_letter == "R":
                    red += 1

        red = min(red, MAX_PLAYERS_PER_TEAM)
        blu = min(blu, MAX_PLAYERS_PER_TEAM)
        r =  {"red": red, "blu": blu}
        return r

    def get_player_counts_for_squares(self, square_ids: List[int]) -> Dict[str, int]:
        """
        Get player counts only for specified control square IDs.

        square_ids: e.g. [1, 2] meaning CS-01, CS-02.
        """
        if not square_ids:
            return {"red": 0, "blu": 0}

        wanted = set(int(s) for s in square_ids)
        red = 0
        blu = 0
        now = self.clock.milliseconds()
        fresh_ms = self.fresh_window_ms

        with self._lock:
            for _, info in self.devices.items():
                if not self._is_control_square(info):
                    continue

                last_seen = info.get("last_seen", 0.0) or 0.0
                age = now - last_seen
                if age > fresh_ms:
                    continue

                square_idx = None
                name = (info.get("name") or "").strip().upper()
                if name.startswith(self._CS_PREFIX):
                    try:
                        square_idx = int(name.split("-")[1])
                    except Exception:
                        square_idx = None

                if square_idx is None:
                    mf = info.get("manufacturer_data") or ""
                    parts = mf.split(",")
                    if len(parts) >= 2:
                        tile = parts[1].strip().upper()
                        if tile.startswith(self._CS_PREFIX):
                            try:
                                square_idx = int(tile.split("-")[1])
                            except Exception:
                                square_idx = None

                if square_idx is None or square_idx not in wanted:
                    continue

                mf = info.get("manufacturer_data") or ""
                team_letter = self.get_player_color(mf)
                if team_letter == "B":
                    blu += 1
                elif team_letter == "R":
                    red += 1

        red = min(red, MAX_PLAYERS_PER_TEAM)
        blu = min(blu, MAX_PLAYERS_PER_TEAM)
        return {"red": red, "blu": blu}


# ======================================================================
# Stand-alone CLI test harness
# ======================================================================

class _MonotonicClock(Clock):
    """Simple clock for CLI testing."""
    def milliseconds(self) -> int:
        return int(time.monotonic() * 1000)


async def _cli_main(adapter_index: int = 1):
    """
    Runs a simple loop printing scanner status every second.
    On Windows: uses Bleak.
    On Linux:   uses bleson on hci<adapter_index>.
    """
    clock = _MonotonicClock()
    scanner = EnhancedBLEScanner(clock, linux_adapter_index=adapter_index)

    print("[BLE TEST] Starting scanner… (Ctrl+C to stop)")
    await scanner.start_scanning()

    try:
        while True:
            await asyncio.sleep(1.0)

            summary = scanner.get_devices_summary()
            counts = scanner.get_player_counts()

            print("\n[BLE TEST] -------------------------------")
            print(f"[BLE TEST] scanning={summary['scanning']}  "
                  f"device_count={summary['device_count']}")
            print(f"[BLE TEST] players: RED={counts['red']}  BLU={counts['blu']}")

            for dev in summary["devices"]:
                name = dev["name"]
                if name.startswith("CS-"):
                    last_gap = dev["last_gap_ms"]
                    last_gap_str = f"{last_gap:4d}ms" if last_gap is not None else "----"
                    print(
                        f"  {name} {dev['address']}  "
                        f"age={dev['last_seen_ms']:4d}ms  "
                        f"rssi={dev['rssi']:4d}  "
                        f"mfg='{dev['manufacturer_data']}'  "
                        f"cb_count={dev['cb_count']:5d}  last_gap={last_gap_str}"
                    )

    except KeyboardInterrupt:
        print("\n[BLE TEST] stopping scanner…")
        await scanner.stop_scanning()


if __name__ == "__main__":
    idx = 1
    if len(sys.argv) >= 2:
        try:
            idx = int(sys.argv[1])
        except ValueError:
            pass

    try:
        asyncio.run(_cli_main(adapter_index=idx))
    except RuntimeError:
        loop = asyncio.get_event_loop()
        loop.run_until_complete(_cli_main(adapter_index=idx))
