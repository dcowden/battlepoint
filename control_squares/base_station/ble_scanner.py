import traceback
from typing import Optional, List, Dict
import asyncio
import sys
import threading
import time
import struct

from battlepoint_core import (
    Clock,
    BluetoothTag,
    Team,
    TagType,
)

MAX_PLAYERS_PER_TEAM = 3

# Only import aioblescan on non-Windows
if sys.platform != "win32":
    import aioblescan as aiobs

try:
    # Optional: keep Bleak for Windows dev
    from bleak import BleakScanner
except ImportError:
    BleakScanner = None


# ============================================================================
# CONFIG / CONSTANTS
# ============================================================================

# We consider something "ours" if its manufacturer ASCII contains a tile id
# like CS-01, CS-02, etc. Either:
#   "CS-02,..."      (tile id first)
#   "B,CS-02,..."    (team, tile id, ...)
_TILE_PREFIX = "CS-"
_TEAM_CHARS = ("B", "R")


class EnhancedBLEScanner:
    _CS_PREFIX = "CS-"
    _PLAYER_PREFIX = "PT-"
    FRESH_WINDOW_MS = 750  # slightly generous; adjust if you want tighter

    def __init__(self, clock: Clock, linux_adapter_index: int = 1):
        """
        linux_adapter_index: HCI index for aioblescan on Linux (default hci1).
        """
        self.clock = clock
        self.devices: Dict[str, dict] = {}
        self.fresh_window_ms = EnhancedBLEScanner.FRESH_WINDOW_MS

        # Common state
        self._lock = threading.Lock()
        self._scanning: bool = False

        # Platform split
        self._is_windows = (sys.platform == "win32")

        # Windows (Bleak) bits
        self._scanner: Optional["BleakScanner"] = None
        self._want_scan: bool = False
        self._thread: Optional[threading.Thread] = None
        self._thread_loop: Optional[asyncio.AbstractEventLoop] = None

        # Linux (aioblescan socket) bits
        self._linux_adapter_index = linux_adapter_index
        self._linux_transport = None
        self._linux_btctrl = None
        self._linux_thread: Optional[threading.Thread] = None
        self._linux_loop: Optional[asyncio.AbstractEventLoop] = None
        self._linux_want_scan: bool = False

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
        Windows path: we still trust Bleak to give us per-advert data.
        """
        name = device.name or ""
        address = device.address
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
    # LINUX PATH — manual HCI parsing
    # ======================================================================

    def _start_linux_thread(self):
        """Start the dedicated Linux scanner thread + event loop."""
        if self._linux_thread is not None:
            return

        def _runner():
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            self._linux_loop = loop
            loop.run_until_complete(self._linux_scanner_main())

        self._linux_thread = threading.Thread(target=_runner, daemon=True)
        self._linux_thread.start()
        print(f"[BLE] Linux scanner thread started on hci{self._linux_adapter_index}")

    async def _linux_scanner_main(self):
        """Runs in the Linux scanner thread's event loop."""
        try:
            sock = aiobs.create_bt_socket(self._linux_adapter_index)
            loop = asyncio.get_event_loop()
            fac = loop._create_connection_transport(  # type: ignore[attr-defined]
                sock, aiobs.BLEScanRequester, None, None
            )
            transport, btctrl = await fac

            self._linux_transport = transport
            self._linux_btctrl = btctrl

            # IMPORTANT: we ignore aioblescan's decoder and parse HCI by hand
            btctrl.process = self._aiobs_process_raw

            while True:
                try:
                    if self._linux_want_scan and not self._scanning:
                        await btctrl.send_scan_request(0)
                        self._scanning = True
                        print(f"[BLE] (linux) scanning started on hci{self._linux_adapter_index}")
                    elif not self._linux_want_scan and self._scanning:
                        try:
                            await btctrl.stop_scan_request()
                        except Exception as e:
                            print(f"[BLE] (linux) stop_scan_request error: {e!r}")
                        self._scanning = False
                        print(f"[BLE] (linux) scanning stopped on hci{self._linux_adapter_index}")
                except Exception as e:
                    print(f"[BLE] (linux) main loop error: {e!r}")

                await asyncio.sleep(0.2)

        finally:
            if self._linux_transport is not None:
                self._linux_transport.close()
            self._linux_transport = None
            self._linux_btctrl = None
            self._scanning = False
            print(f"[BLE] (linux) scanner thread exiting on hci{self._linux_adapter_index}")

    # ---------- Raw HCI parser ----------

    def _aiobs_process_raw(self, data: bytes) -> None:
        """
        Called by aioblescan BLEScanRequester with raw HCI event bytes.

        We ignore aioblescan's high-level decode and instead parse LE
        Advertising Report (0x02) and LE Extended Advertising Report (0x0D)
        manually to avoid the "mixed mfg data across devices" bug.
        """
        try:
            if not data or len(data) < 3:
                return

            # HCI LE Meta Event format used by aioblescan:
            #   byte 0: event code (0x3E)
            #   byte 1: parameter length
            #   byte 2: subevent code (0x02 adv report, 0x0D ext adv report, ...)
            event_code = data[0]
            if event_code != 0x3E:
                return

            param_len = data[1]
            # Make sure we don't walk past the buffer if kernel lies
            if 2 + param_len > len(data):
                param_len = len(data) - 2

            subevent = data[2]
            now_ms = self.clock.milliseconds()

            if subevent == 0x02:
                self._parse_legacy_adv_reports(data, 3, now_ms)
            elif subevent == 0x0D:
                self._parse_ext_adv_reports(data, 3, now_ms)
            else:
                # Ignore other LE Meta subevents
                return

        except Exception as e:
            print(f"[BLE RAW] parse error: {e!r}")
            # Debug: uncomment to see raw
            # print("RAW:", data.hex())

    def _parse_legacy_adv_reports(self, buf: bytes, offset: int, now_ms: int) -> None:
        """
        Parse LE Advertising Report (subevent 0x02).

        Layout (per spec):
            num_reports: 1 byte
            For each report:
              event_type: 1 byte
              address_type: 1 byte
              address: 6 bytes (LSB first)
              data_len: 1 byte
              data: data_len bytes
              rssi: 1 byte (signed)
        """
        if offset >= len(buf):
            return

        num_reports = buf[offset]
        offset += 1

        for _ in range(num_reports):
            if offset + 9 > len(buf):
                break

            evt_type = buf[offset]
            addr_type = buf[offset + 1]
            addr_bytes = buf[offset + 2 : offset + 8]
            offset += 8

            data_len = buf[offset]
            offset += 1

            if offset + data_len + 1 > len(buf):
                break

            adv_data = buf[offset : offset + data_len]
            offset += data_len

            rssi_raw = buf[offset]
            offset += 1
            rssi = struct.unpack("b", bytes([rssi_raw]))[0]

            address = ":".join(f"{b:02x}" for b in addr_bytes[::-1])

            mfg_ascii = self._extract_mfg_from_adv_data(adv_data)

            print(f"[BLE RAW-L] peer={address} rssi={rssi:4d} mfg='{mfg_ascii}'")

            tile_id, logical_name = self._extract_tile_from_mfg(mfg_ascii)
            if tile_id is None:
                continue  # not one of ours

            self._record_observation(
                address=address,
                logical_name=logical_name,
                rssi=rssi,
                mfg_ascii=mfg_ascii,
                now_ms=now_ms,
            )

    def _parse_ext_adv_reports(self, buf: bytes, offset: int, now_ms: int) -> None:
        """
        Parse LE Extended Advertising Report (subevent 0x0D).

        Layout (per spec):
            num_reports: 1 byte
            For each report:
              event_type: 2 bytes (LE Extended Adv Event Type)
              address_type: 1 byte
              address: 6 bytes (LSB first)
              primary_phy: 1
              secondary_phy: 1
              adv_sid: 1
              tx_power: 1
              rssi: 1 (signed, 127 means 'not available')
              periodic_adv_interval: 2
              direct_addr_type: 1
              direct_addr: 6
              data_len: 1
              data: data_len bytes
        """
        if offset >= len(buf):
            return

        num_reports = buf[offset]
        offset += 1

        for _ in range(num_reports):
            # Fixed header size before data_len: 2 + 1 + 6 + 1 + 1 + 1 + 1 + 1 + 2 + 1 + 6 + 1 = 24
            if offset + 24 > len(buf):
                break

            evt_type = buf[offset] | (buf[offset + 1] << 8)
            offset += 2

            addr_type = buf[offset]
            offset += 1

            addr_bytes = buf[offset : offset + 6]
            offset += 6

            primary_phy = buf[offset]
            secondary_phy = buf[offset + 1]
            offset += 2

            adv_sid = buf[offset]
            offset += 1

            tx_power = buf[offset]
            offset += 1

            rssi_raw = buf[offset]
            offset += 1
            # 127 == "RSSI not available"
            if rssi_raw == 127:
                rssi = 0
            else:
                rssi = struct.unpack("b", bytes([rssi_raw]))[0]

            periodic_interval = buf[offset] | (buf[offset + 1] << 8)
            offset += 2

            direct_addr_type = buf[offset]
            offset += 1

            direct_addr_bytes = buf[offset : offset + 6]
            offset += 6

            if offset >= len(buf):
                break

            data_len = buf[offset]
            offset += 1

            if offset + data_len > len(buf):
                break

            adv_data = buf[offset : offset + data_len]
            offset += data_len

            address = ":".join(f"{b:02x}" for b in addr_bytes[::-1])

            mfg_ascii = self._extract_mfg_from_adv_data(adv_data)

            print(f"[BLE RAW-E] peer={address} rssi={rssi:4d} mfg='{mfg_ascii}'")

            tile_id, logical_name = self._extract_tile_from_mfg(mfg_ascii)
            if tile_id is None:
                continue  # not one of ours

            self._record_observation(
                address=address,
                logical_name=logical_name,
                rssi=rssi,
                mfg_ascii=mfg_ascii,
                now_ms=now_ms,
            )

    # ---------- AD & manufacturer parsing ----------

    def _extract_mfg_from_adv_data(self, adv_data: bytes) -> str:
        """
        Parse the AD structures (Length, Type, Value...) and return the first
        Manufacturer Specific Data (type 0xFF) as ASCII, or "" if none.
        """
        i = 0
        n = len(adv_data)
        while i < n:
            length = adv_data[i]
            if length == 0:
                break
            if i + length >= n:
                break

            ad_type = adv_data[i + 1]
            value = adv_data[i + 2 : i + 1 + length]

            if ad_type == 0xFF:  # Manufacturer Specific Data
                # First 2 bytes are company ID (little-endian)
                if len(value) >= 2:
                    payload = value[2:]
                else:
                    payload = b""
                try:
                    return payload.decode("ascii", errors="ignore")
                except Exception:
                    return payload.hex()

            i += length + 1

        return ""

    def _extract_tile_from_mfg(self, mfg_ascii: str) -> (Optional[str], Optional[str]):
        """
        From manufacturer ASCII, decide if this is one of our tiles and
        return (tile_id, logical_name).

        We support two formats:
          1) "CS-02,..."      -> tile_id="CS-02"
          2) "B,CS-02,..."    -> team, tile_id, ...
        """
        if not mfg_ascii:
            return None, None

        s = mfg_ascii.strip()
        tile_id = None

        # Format 1: "CS-02,...."
        if s.startswith(_TILE_PREFIX):
            tile_id = s.split(",", 1)[0].strip()

        # Format 2: "B,CS-02,PT-...,..." etc.
        if tile_id is None:
            parts = s.split(",")
            if len(parts) >= 2:
                team = parts[0].strip().upper()
                maybe_tile = parts[1].strip().upper()
                if maybe_tile.startswith(_TILE_PREFIX):
                    tile_id = maybe_tile

        if tile_id is None:
            return None, None

        logical_name = tile_id  # what we show as "name"
        return tile_id, logical_name

    # ======================================================================
    # SHARED RECORD / SUMMARY
    # ======================================================================

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
        if self._is_windows:
            self._want_scan = True
            return

        self._linux_want_scan = True
        if self._linux_thread is None:
            self._start_linux_thread()

    async def stop_scanning(self):
        if self._is_windows:
            self._want_scan = False
            return

        self._linux_want_scan = False

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

        team = parts[0].strip().upper()
        if team in ("B", "R"):
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
        return {"red": red, "blu": blu}

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
    On Linux:   uses raw HCI parsing on hci<adapter_index>.
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
