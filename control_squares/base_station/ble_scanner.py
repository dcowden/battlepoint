from typing import Optional, List, Dict
import asyncio
import re
import sys
import threading
import time

from bleak import BleakScanner

from battlepoint_core import (
    Clock,
    BluetoothTag,
)


MAX_PLAYERS_PER_TEAM = 3


class EnhancedBLEScanner:
    _CS_PREFIX = "CS-"
    FRESH_WINDOW_MS = 750

    def __init__(self, clock: Clock):
        self.clock = clock
        self.devices: Dict[str, dict] = {}
        self.fresh_window_ms = EnhancedBLEScanner.FRESH_WINDOW_MS

        self._scanner: Optional[BleakScanner] = None
        self._scanning: bool = False
        self._want_scan: bool = False
        self._lock = threading.Lock()

        self._is_windows = (sys.platform == "win32")
        self._thread: Optional[threading.Thread] = None
        self._thread_loop: Optional[asyncio.AbstractEventLoop] = None

        # DEBUG: track last callback time per address for gap measurement
        self._last_cb_time_by_addr: Dict[str, int] = {}

        if self._is_windows:
            self._start_windows_thread()

    # ---------- Windows thread ----------

    def _start_windows_thread(self):
        if self._thread is not None:
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
        # NOTE: for Windows we’re just using default BleakScanner config here.
        # Later, on Linux, you can change this to include bluez filters.
        self._scanner = BleakScanner(detection_callback=self._detection_callback)

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

    # ---------- Common callback ----------

    def _detection_callback(self, device, advertisement_data):
        name = device.name
        if not name:
            return

        address = device.address
        rssi = advertisement_data.rssi
        current_time = self.clock.milliseconds()

        mfg_data_str = "N/A"
        if advertisement_data.manufacturer_data:
            for _, data_bytes in advertisement_data.manufacturer_data.items():
                try:
                    mfg_data_str = data_bytes.decode("ascii")
                except UnicodeDecodeError:
                    mfg_data_str = data_bytes.hex()
                break

        # DEBUG: compute per-address callback gaps
        prev = self._last_cb_time_by_addr.get(address)
        if prev is not None:
            gap = current_time - prev
            # only log aggressively for our control squares to keep noise down
            if name.startswith(self._CS_PREFIX):
                # truncate mf for readability
                mf_short = (
                    mfg_data_str
                    if len(mfg_data_str) <= 40
                    else mfg_data_str[:37] + "..."
                )
                print(
                    f"[BLE] cb addr={address} name={name} "
                    f"gap={gap:4d}ms rssi={rssi:4d} mf={mf_short}"
                )
        else:
            if name.startswith(self._CS_PREFIX):
                print(
                    f"[BLE] first cb addr={address} name={name} "
                    f"rssi={rssi:4d} mf={mfg_data_str}"
                )
        self._last_cb_time_by_addr[address] = current_time

        with self._lock:
            self.devices[address] = {
                "name": name,
                "rssi": rssi,
                "address": address,
                "manufacturer_data": mfg_data_str,
                "last_seen": current_time,
            }

    # ---------- Public API ----------

    async def start_scanning(self):
        if self._is_windows:
            self._want_scan = True
            return

        if self._scanning:
            return
        try:
            # On Linux you can tweak BleakScanner config here (see notes below)
            self._scanner = BleakScanner(
                detection_callback=self._detection_callback,
                scanning_mode="active",
                bluez={
                    "filters": {
                        "DuplicateData": True,
                        # optionally: "Transport": "le", "DuplicateData": True
                    }
                },
            )
            await self._scanner.start()
            self._scanning = True
            print("[BLE] (linux) scanning started")
        except Exception as e:
            print(f"[BLE] (linux) error starting scan: {e!r}")

    async def stop_scanning(self):
        if self._is_windows:
            self._want_scan = False
            return

        if not self._scanning or not self._scanner:
            return
        try:
            await self._scanner.stop()
        finally:
            self._scanning = False
            self._scanner = None
            print("[BLE] (linux) scanning stopped")

    # ---------- Read methods ----------

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

        return {
            "scanning": (self._want_scan if self._is_windows else self._scanning),
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
            (B|R),NAME,PT-NN,1234

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

    # ---------- Player count logic ----------

    def get_player_counts(self) -> dict:
        red = 0
        blu = 0
        now = self.clock.milliseconds()
        fresh_ms = self.fresh_window_ms

        with self._lock:
            for _, info in self.devices.items():
                name = info.get("name") or ""
                if not name.startswith(self._CS_PREFIX):
                    # Only CS-0X devices contribute to counts
                    continue

                last_seen = info.get("last_seen", 0.0) or 0.0
                age = now - last_seen
                if (now - last_seen) > fresh_ms:
                    print(
                        f"[BLE] stale tile {name}: age={age}ms > {fresh_ms}ms"
                    )
                    # Ignore stale tiles
                    continue

                mf = info.get("manufacturer_data") or ""
                team_letter = self.get_player_color(mf)
                if team_letter is None:
                    print(f"[BLE] no team from mf='{mf}' for {name}")
                    pass
                if team_letter == "B":
                    blu += 1
                elif team_letter == "R":
                    red += 1

        red = min(red, MAX_PLAYERS_PER_TEAM)
        blu = min(blu, MAX_PLAYERS_PER_TEAM)
        return {"red": red, "blu": blu}


# ---------- Simple clock for CLI testing ----------

class MonotonicClock(Clock):
    """Clock implementation backed by time.monotonic() for debugging."""

    def milliseconds(self) -> int:
        return int(time.monotonic() * 1000)


# ---------- CLI harness ----------

async def _cli_main():
    clock = MonotonicClock()
    scanner = EnhancedBLEScanner(clock)

    print("[BLE] starting scanning (Ctrl+C to stop)")
    await scanner.start_scanning()

    try:
        while True:
            await asyncio.sleep(1.0)
            summary = scanner.get_devices_summary()
            print(
                f"[BLE] summary: scanning={summary['scanning']} "
                f"devices={summary['device_count']}"
            )
            # Focus on your tiles
            for dev in summary["devices"]:
                if dev["name"].startswith("CS-"):
                    print(
                        f"    {dev['name']} {dev['address']} "
                        f"age={dev['last_seen_ms']:4d}ms "
                        f"rssi={dev['rssi']:4d}"
                    )
    except KeyboardInterrupt:
        print("\n[BLE] stopping...")
        await scanner.stop_scanning()


if __name__ == "__main__":
    asyncio.run(_cli_main())
