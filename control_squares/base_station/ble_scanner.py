import asyncio
import sys
import time
from bleak import BleakScanner
from typing import Dict, Optional, Callable

# Windows-specific fix
if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())


class BLEScanner:
    """A BLE scanner class that continuously monitors devices with real-time updates."""

    def __init__(self):
        self.devices: Dict[str, dict] = {}
        self._scanner: Optional[BleakScanner] = None
        self._scanning = False

    def _detection_callback(self, device, advertisement_data):
        """Callback function called every time a device is detected."""
        name = device.name
        if not name:  # Skip devices without names
            return

        address = device.address
        rssi = advertisement_data.rssi
        current_time = time.time()

        # Extract manufacturer data as string
        mfg_data_str = "N/A"
        if advertisement_data.manufacturer_data:
            for company_id, data_bytes in advertisement_data.manufacturer_data.items():
                try:
                    mfg_data_str = data_bytes.decode('ascii')
                except UnicodeDecodeError:
                    mfg_data_str = data_bytes.hex()
                break

        # Update device info
        self.devices[address] = {
            'name': name,
            'rssi': rssi,
            'address': address,
            'manufacturer_data': mfg_data_str,
            'last_seen': current_time
        }

    async def start_continuous_scan(self):
        """Start continuous scanning in the background."""
        if self._scanning:
            print("Already scanning!")
            return

        print("Starting continuous BLE scan...")
        self._scanning = True

        try:
            self._scanner = BleakScanner(detection_callback=self._detection_callback)
            await self._scanner.start()
            print("Continuous scanning started!")
        except OSError as e:
            print(f"Error starting scan: {e}")
            print("\nTroubleshooting steps:")
            print("1. Make sure Bluetooth is enabled in Windows Settings")
            print("2. Try running this script as Administrator")
            self._scanning = False

    async def stop_continuous_scan(self):
        """Stop the continuous scan."""
        if not self._scanning or not self._scanner:
            return

        print("\nStopping continuous scan...")
        await self._scanner.stop()
        self._scanning = False
        self._scanner = None

    def get_all_devices(self) -> Dict[str, dict]:
        """Get all currently tracked devices."""
        return self.devices

    def get_device_by_name(self, name: str) -> Optional[dict]:
        """Find a device by its name."""
        for device in self.devices.values():
            if device['name'] == name:
                return device
        return None

    def get_device_by_address(self, address: str) -> Optional[dict]:
        """Find a device by its address."""
        return self.devices.get(address)

    def get_strongest_device(self) -> Optional[dict]:
        """Get the device with the strongest signal (highest RSSI)."""
        if not self.devices:
            return None
        return max(self.devices.values(), key=lambda d: d['rssi'])

    def get_time_since_last_seen(self, address: str) -> Optional[float]:
        """Get milliseconds since device was last seen."""
        device = self.devices.get(address)
        if not device:
            return None
        return (time.time() - device['last_seen']) * 1000

    def print_devices(self):
        """Print all tracked devices with current time since last seen."""
        current_time = time.time()
        print("\n" + "=" * 120)

        if not self.devices:
            print("No devices detected yet...")
        else:
            for address, device_info in self.devices.items():
                time_since = int((current_time - device_info['last_seen']) * 1000)
                print(f"Device: {device_info['name']:30} | RSSI: {device_info['rssi']:4} dBm | "
                      f"Address: {device_info['address']:20} | Last seen: {time_since:6} ms ago | "
                      f"Mfg Data: {device_info['manufacturer_data']}")

        print("=" * 120)
        print(f"Tracking {len(self.devices)} device(s)")


# Example usage with continuous display updates
async def main():
    scanner = BLEScanner()

    # Start continuous scanning
    await scanner.start_continuous_scan()

    try:
        # Update display every 100ms for real-time monitoring
        while True:
            await asyncio.sleep(0.1)  # 100ms update interval
            scanner.print_devices()

    except KeyboardInterrupt:
        print("\n\nStopping...")
    finally:
        await scanner.stop_continuous_scan()


# Alternative: Simple continuous scan without display loop
async def simple_scan():
    scanner = BLEScanner()
    await scanner.start_continuous_scan()

    try:
        # Just let it run and accumulate data
        # You can query scanner.devices or use get methods whenever needed
        while True:
            await asyncio.sleep(1)
            print(f"\nCurrently tracking {len(scanner.devices)} devices")

            # Example: Get a specific device
            sq01 = scanner.get_device_by_name("SQ-01")
            if sq01:
                time_since = scanner.get_time_since_last_seen(sq01['address'])
                print(f"SQ-01 last seen {time_since:.0f}ms ago, RSSI: {sq01['rssi']}")

    except KeyboardInterrupt:
        print("\n\nStopping...")
    finally:
        await scanner.stop_continuous_scan()


if __name__ == "__main__":
    # Run the main example with continuous display
    asyncio.run(main())

    # Or run the simple example
    # asyncio.run(simple_scan())