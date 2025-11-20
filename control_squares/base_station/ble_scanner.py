import asyncio
import sys
import threading
import time
from typing import Optional, List, Dict

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


class EnhancedBLEScanner:
    _CS_PREFIX = "CS-"
    FRESH_WINDOW_MS = 750  # ms window to consider a tile "fresh"

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

        # Linux (aioblescan) bits
        self._linux_adapter_index = linux_adapter_index
        self._linux_transport = None
        self._linux_btctrl = None
        self._linux_thread: Optional[threading.Thread] = None
        self._linux_loop: Optional[asyncio.AbstractEventLoop] = None
        self._linux_want_scan: bool = False

        # DEBUG: track last callback time per address for gap measurement
        self._last_cb_time_by_addr: Dict[str, int] = {}

        if self._is_windows:
            self._start_windows_thread()

    # ======================================================================
    # UTILS: manufacturer parsing
    # ======================================================================

    @staticmethod
    def _decode_mfg_bytes(raw: bytes) -> str:
        """Decode manufacturer bytes as ASCII (fallback to hex)."""
        if not raw:
            return ""
        try:
            return raw.decode("ascii", errors="ignore")
        except Exception:
            return raw.hex()

    @classmethod
    def _tile_id_from_mfg(cls, mfg_ascii: str) -> Optional[str]:
        """
        Extract tile id from manufacturer string.

        We look for any comma-separated token that starts with "CS-",
        e.g. "B,CS-02,PT-UNK,1234" -> "CS-02".
        """
        if not mfg_ascii:
            return None
        parts = mfg_ascii.split(",")
        for p in parts:
            token = p.strip().upper()
            if token.startswith(cls._CS_PREFIX):
                return token  # we store tile_id uppercase for consistency
        return None

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
        Windows path: only cares about manufacturer data.
        If manufacturer segment contains a CS-xx token, it's ours.
        """
        address = device.address
        rssi = advertisement_data.rssi
        mfg_ascii = ""

        if advertisement_data.manufacturer_data:
            # Grab first entry
            for _, data_bytes in advertisement_data.manufacturer_data.items():
                mfg_ascii = self._decode_mfg_bytes(data_bytes)
                break

        tile_id = self._tile_id_from_mfg(mfg_ascii)
        if not tile_id:
            # Not a control square
            return

        now_ms = self.clock.milliseconds()

        # Raw debug
        print(f"[BLE RAW] peer={address} rssi={rssi:4d} mf='{mfg_ascii}' tile_id={tile_id}")

        self._record_observation(
            address=address,
            tile_name=tile_id,  # we use tile_id as the "name"
            rssi=rssi,
            mfg_ascii=mfg_ascii,
            now_ms=now_ms,
        )

    # ======================================================================
    # LINUX PATH (aioblescan)
    # ======================================================================

    def _decode_aiobs_mfg(self, mfg_obj) -> str:
        """
        Extract ASCII manufacturer data from aioblescan Manufacturer Specific Data.
        """
        if mfg_obj is None:
            return ""

        payload = getattr(mfg_obj, "payload", None)

        raw = None
        if isinstance(payload, (bytes, bytearray)):
            raw = bytes(payload)
        elif isinstance(payload, list):
            # Typical aioblescan structure: [company_id, Itself(raw_bytes)]
            for part in payload:
                if hasattr(part, "payload") and isinstance(part.payload, (bytes, bytearray)):
                    raw = bytes(part.payload)
                    break
                if hasattr(part, "val") and isinstance(part.val, (bytes, bytearray)):
                    raw = bytes(part.val)
                    break

        if raw is None:
            return ""

        return self._decode_mfg_bytes(raw)

    def _aiobs_process(self, data: bytes) -> None:
        """
        aioblescan callback (Linux), **manufacturer-only** approach.

        - Decode HCI event.
        - Iterate each sub-event in ev.events (or ev itself if no sub-events).
        - For each sub-event:
            * Decode manufacturer payload (from that subevent).
            * If it doesn't contain CS-xx, ignore it.
            * Get peer address + RSSI (from that subevent).
            * Record observation.
        """
        try:
            ev = aiobs.HCI_Event()
            ev.decode(data)
        except Exception as e:
            print(f"[BLE] aioblescan decode error: {e!r}")
            return

        subevents = getattr(ev, "events", None)
        if not subevents:
            subevents = [ev]

        for pkt in subevents:
            try:
                # Manufacturer data per subevent
                mfg_ascii = ""
                mfg_items = pkt.retrieve("Manufacturer Specific Data")
                if mfg_items:
                    mfg_ascii = self._decode_aiobs_mfg(mfg_items[0])

                # If no manufacturer data, ignore this subevent
                if not mfg_ascii:
                    continue

                # Extract tile id purely from manufacturer segment
                tile_id = self._tile_id_from_mfg(mfg_ascii)
                if not tile_id:
                    # Not one of our control squares
                    continue

                # MAC address per subevent
                peer = pkt.retrieve("peer")
                if not peer:
                    continue
                address = peer[0].val.lower()

                # RSSI per subevent
                rssi_items = pkt.retrieve("rssi")
                rssi = rssi_items[0].val if rssi_items else 0

                # Raw debug for every **tile** advert
                print(f"[BLE RAW] peer={address} rssi={rssi:4d} mf='{mfg_ascii}' tile_id={tile_id}")

                now_ms = self.clock.milliseconds()
                self._record_observation(
                    address=address,
                    tile_name=tile_id,  # just use tile_id as the name
                    rssi=rssi,
                    mfg_ascii=mfg_ascii,
                    now_ms=now_ms,
                )

            except Exception as e:
                print(f"[BLE] aioblescan subevent error: {e!r}")

    def _start_linux_thread(self):
        """Start the dedicated Linux aioblescan thread + event loop."""
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
            btctrl.process = self._aiobs_process

            while True:
                try:
                    if self._linux_want_scan and not self._scanning:
                        await btctrl.send_scan_request(0)
                        self._scanning = True
                        print(f"[BLE] (linux/aioblescan) scanning started on hci{self._linux_adapter_index}")
                    elif not self._linux_want_scan and self._scanning:
                        try:
                            await btctrl.stop_scan_request()
                        except Exception as e:
                            print(f"[BLE] (linux/aioblescan) stop_scan_request error: {e!r}")
                        self._scanning = False
                        print(f"[BLE] (linux/aioblescan) scanning stopped on hci{self._linux_adapter_index}")
                except Exception as e:
                    print(f"[BLE] (linux/aioblescan) main loop error: {e!r}")

                await asyncio.sleep(0.2)

        finally:
            if self._linux_transport is not None:
                self._linux_transport.close()
            self._linux_transport = None
            self._linux_btctrl = None
            self._scanning = False
            print(f"[BLE] (linux/aioblescan) scanner thread exiting on hci{self._linux_adapter_index}")

    # ======================================================================
    # RECORD OBSERVATIONS
    # ======================================================================

    def _record_observation(self, address: str, tile_name: str, rssi: int, mfg_ascii: str, now_ms: int):
        """
        Shared between Linux (aioblescan) and Windows (Bleak) paths.
        This is where we update self.devices and debug-log gaps.

        - tile_name is already the tile_id (e.g., "CS-02").
        - We track cb_count and last_gap_ms per MAC.
        """

        # 1) Per-address callback gaps
        prev = self._last_cb_time_by_addr.get(address)
        gap = None
        if prev is not None:
            gap = now_ms - prev
            print(
                f"[BLE] cb addr={address} name={tile_name} "
                f"gap={gap:4d}ms rssi={rssi:4d} mf='{mfg_ascii}'"
            )
        else:
            print(
                f"[BLE] first cb addr={address} name={tile_name} "
                f"rssi={rssi:4d} mf='{mfg_ascii}'"
            )

        self._last_cb_time_by_addr[address] = now_ms

        # 2) Store/update device record
        with self._lock:
            info = self.devices.get(address)
            if info is None:
                info = {
                    "name": tile_name,
                    "rssi": rssi,
                    "address": address,
                    "manufacturer_data": mfg_ascii,
                    "last_seen": now_ms,
                    "cb_count": 0,
                    "last_gap_ms": None,
                }
                self.devices[address] = info

            # update dynamic fields
            info["name"] = tile_name
            info["rssi"] = rssi
            info["manufacturer_data"] = mfg_ascii
            info["last_seen"] = now_ms
            info["cb_count"] = info.get("cb_count", 0) + 1
            if gap is not None:
                info["last_gap_ms"] = gap

    # ======================================================================
    # PUBLIC API: start/stop
    # ======================================================================

    async def start_scanning(self):
        if self._is_windows:
            # Windows → tell the Bleak thread to start
            self._want_scan = True
            return

        # Linux: run aioblescan in its own thread/loop
        self._linux_want_scan = True
        if self._linux_thread is None:
            self._start_linux_thread()

    async def stop_scanning(self):
        if self._is_windows:
            self._want_scan = False
            return

        # Linux: just flip the flag; the Linux scanner thread will stop the scan
        self._linux_want_scan = False

    # ======================================================================
    # READ METHODS / GAME INTEGRATION
    # ======================================================================

    def get_devices_summary(self) -> dict:
        with self._lock:
            current_time = self.clock.milliseconds()
            device_list = []
            for address, info in self.devices.items():
                device_list.append(
                    {
                        "name": info["name"],
                        "rssi": info["rssi"],
                        "address": address,
                        "last_seen_ms": int(current_time - info["last_seen"]),
                        "manufacturer_data": info["manufacturer_data"],
                        "cb_count": info.get("cb_count", 0),
                        "last_gap_ms": info.get("last_gap_ms"),
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

    # ---------- Helper parsing ----------

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

        Now purely name-based: name is tile_id derived from manufacturer.
        """
        name = (info.get("name") or "").strip().upper()
        return name.startswith(self._CS_PREFIX)

    # ---------- Player count logic ----------

    def get_player_counts(self) -> dict:
        red = 0
        blu = 0
        now = self.clock.milliseconds()
        fresh_ms = self.fresh_window_ms

        with self._lock:
            for _, info in self.devices.items():
                if not self._is_control_square(info):
                    # Only control squares contribute to counts
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
                # Only control squares
                if not self._is_control_square(info):
                    continue

                last_seen = info.get("last_seen", 0.0) or 0.0
                age = now - last_seen
                if age > fresh_ms:
                    continue

                # Determine square index from name (CS-xx)
                square_idx = None
                name = (info.get("name") or "").strip().upper()
                if name.startswith(self._CS_PREFIX):
                    try:
                        square_idx = int(name.split("-")[1])
                    except Exception:
                        square_idx = None

                if square_idx is None or square_idx not in wanted:
                    continue

                # Count team from manufacturer data
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
    On Linux:   uses aioblescan on hci<adapter_index>.
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

            # Show only CS-* tiles for clarity
            for dev in summary["devices"]:
                name = dev["name"]
                if name.startswith("CS-"):
                    cb_count = dev.get("cb_count", 0)
                    last_gap = dev.get("last_gap_ms", None)
                    lg = f"{last_gap:4d}ms" if last_gap is not None else "----"
                    print(
                        f"  {name} {dev['address']}  "
                        f"age={dev['last_seen_ms']:4d}ms  "
                        f"rssi={dev['rssi']:4d}  "
                        f"mfg='{dev['manufacturer_data']}'  "
                        f"cb_count={cb_count:5d}  last_gap={lg}"
                    )

    except KeyboardInterrupt:
        print("\n[BLE TEST] stopping scanner…")
        await scanner.stop_scanning()


if __name__ == "__main__":
    # Allow: python ble_scanner.py
    # Or:    python ble_scanner.py 0   to use hci0 instead of hci1.
    idx = 1
    if len(sys.argv) >= 2:
        try:
            idx = int(sys.argv[1])
        except ValueError:
            pass

    try:
        asyncio.run(_cli_main(adapter_index=idx))
    except RuntimeError:
        # Allows running from interactive event loops
        loop = asyncio.get_event_loop()
        loop.run_until_complete(_cli_main(adapter_index=idx))
