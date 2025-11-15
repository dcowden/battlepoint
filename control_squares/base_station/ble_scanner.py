from typing import Optional, List, Dict
import asyncio
import re
import sys
import threading
import time

from battlepoint_core import (
    Clock,
    BluetoothTag,
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

        # Linux (aioblescan) bits
        self._linux_adapter_index = linux_adapter_index
        self._linux_transport = None
        self._linux_btctrl = None

        # DEBUG: track last callback time per address for gap measurement
        self._last_cb_time_by_addr: Dict[str, int] = {}

        if self._is_windows:
            self._start_windows_thread()

    # ======================================================================
    # WINDOWS PATH (Bleak, same idea as your old implementation)
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
        Only used on Windows dev boxes. Mirrors old logic as much as possible,
        but parses manufacturer data into ASCII so the rest of the class
        behaves the same as the Linux path.
        """
        name = device.name or ""
        address = device.address
        rssi = advertisement_data.rssi
        current_time = self.clock.milliseconds()

        mfg_ascii = ""
        if advertisement_data.manufacturer_data:
            # Grab first entry
            for _, data_bytes in advertisement_data.manufacturer_data.items():
                try:
                    mfg_ascii = data_bytes.decode("ascii", errors="ignore")
                except Exception:
                    mfg_ascii = data_bytes.hex()
                break

        self._record_observation(
            address=address,
            tile_name=name,        # might be empty on Windows too, but ok
            rssi=rssi,
            mfg_ascii=mfg_ascii,
            now_ms=current_time,
        )

    # ======================================================================
    # LINUX PATH (aioblescan)
    # ======================================================================

    def _decode_aiobs_mfg(self, mfg_obj) -> str:
        """
        Extract ASCII manufacturer data from aioblescan Manufacturer Specific Data.
        We expect payload like "B,CS-02,PT-UNK,4341".
        """
        if mfg_obj is None:
            return ""

        payload = getattr(mfg_obj, "payload", None)

        # raw bytes case
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

        try:
            return raw.decode("ascii", errors="ignore")
        except Exception:
            return raw.hex()

    def _aiobs_process(self, data: bytes) -> None:
        """
        aioblescan callback (Linux).
        """
        try:
            ev = aiobs.HCI_Event()
            ev.decode(data)
        except Exception as e:
            print(f"[BLE] aioblescan decode error: {e!r}")
            return

        # MAC address
        peer = ev.retrieve("peer")
        if not peer:
            return
        address = peer[0].val.lower()

        # RSSI
        rssi_items = ev.retrieve("rssi")
        rssi = rssi_items[0].val if rssi_items else 0

        # Local name is usually empty for your tiles; ignore for logic
        name_items = ev.retrieve("Complete Local Name") or ev.retrieve("Short Local Name")
        tile_name = name_items[0].val if name_items else ""

        # Manufacturer data
        mfg_items = ev.retrieve("Manufacturer Specific Data")
        mfg_ascii = ""
        if mfg_items:
            mfg_ascii = self._decode_aiobs_mfg(mfg_items[0])

        now_ms = self.clock.milliseconds()
        self._record_observation(
            address=address,
            tile_name=tile_name,
            rssi=rssi,
            mfg_ascii=mfg_ascii,
            now_ms=now_ms,
        )

    def _record_observation(self, address: str, tile_name: str, rssi: int, mfg_ascii: str, now_ms: int):
        """
        Shared between Linux (aioblescan) and Windows (Bleak) paths.
        This is where we update self.devices and debug-log gaps.
        """
        # Derive a "logical name" from manufacturer data if needed
        # Format: B,CS-02,PT-UNK,4341 → team, tile_id, ...
        logical_name = tile_name
        if not logical_name and mfg_ascii:
            parts = mfg_ascii.split(",")
            if len(parts) >= 2:
                logical_name = parts[1].strip()  # CS-02

        # DEBUG: per-address callback gaps
        prev = self._last_cb_time_by_addr.get(address)
        if prev is not None:
            gap = now_ms - prev
            # Only log aggressively for tiles (CS-*) to keep noise down
            if logical_name.startswith(self._CS_PREFIX):
                print(
                    f"[BLE] cb addr={address} name={logical_name or tile_name} "
                    f"gap={gap:4d}ms rssi={rssi:4d} mf='{mfg_ascii}'"
                )
        else:
            if logical_name.startswith(self._CS_PREFIX):
                print(
                    f"[BLE] first cb addr={address} name={logical_name or tile_name} "
                    f"rssi={rssi:4d} mf='{mfg_ascii}'"
                )

        self._last_cb_time_by_addr[address] = now_ms

        with self._lock:
            self.devices[address] = {
                "name": logical_name or tile_name,
                "rssi": rssi,
                "address": address,
                "manufacturer_data": mfg_ascii,
                "last_seen": now_ms,
            }

    # ======================================================================
    # PUBLIC API: start/stop
    # ======================================================================

    async def start_scanning(self):
        if self._is_windows:
            # Windows → tell the Bleak thread to start
            self._want_scan = True
            return

        if self._scanning:
            return

        # Linux + aioblescan
        try:
            loop = asyncio.get_running_loop()
            sock = aiobs.create_bt_socket(self._linux_adapter_index)

            # Use the same private helper that aioblescan examples use
            fac = getattr(loop, "_create_connection_transport")(
                sock, aiobs.BLEScanRequester, None, None
            )
            transport, btctrl = await fac

            self._linux_transport = transport
            self._linux_btctrl = btctrl

            # Attach callback
            btctrl.process = self._aiobs_process

            # 0 = passive scan; 1 = active scan
            await btctrl.send_scan_request(0)
            self._scanning = True
            print(f"[BLE] (linux/aioblescan) scanning started on hci{self._linux_adapter_index}")
        except Exception as e:
            print(f"[BLE] (linux/aioblescan) error starting scan: {e!r}")
            self._scanning = False
            if self._linux_transport:
                self._linux_transport.close()
            self._linux_transport = None
            self._linux_btctrl = None

    async def stop_scanning(self):
        if self._is_windows:
            self._want_scan = False
            return

        if not self._scanning:
            return

        try:
            if self._linux_btctrl is not None:
                try:
                    await self._linux_btctrl.stop_scan_request()
                except Exception as e:
                    print(f"[BLE] (linux/aioblescan) stop_scan_request error: {e!r}")
        finally:
            if self._linux_transport is not None:
                self._linux_transport.close()
            self._linux_transport = None
            self._linux_btctrl = None
            self._scanning = False
            print(f"[BLE] (linux/aioblescan) scanning stopped on hci{self._linux_adapter_index}")

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

        On Bleak we *might* have a local name CS-02.
        On aioblescan we often have no name and only manufacturer data.
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
                # also sync the "name" field for UI/debug
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
                    # Only control squares contribute to counts
                    continue

                last_seen = info.get("last_seen", 0.0) or 0.0
                age = now - last_seen
                if age > fresh_ms:
                    # Ignore stale tiles (debug log if you want)
                    # print(f"[BLE] stale tile {info['name']}: age={age}ms > {fresh_ms}ms")
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
                    print(
                        f"  {name} {dev['address']}  "
                        f"age={dev['last_seen_ms']:4d}ms  "
                        f"rssi={dev['rssi']:4d}  "
                        f"mfg='{dev['manufacturer_data']}'"
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
