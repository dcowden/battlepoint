
# BattlePoint - Real-Life Team Fortress 2 Game

A Python/NiceGUI implementation of a real-life Team Fortress 2 style capture point game using Raspberry Pi, Bluetooth tags, and physical equipment.

## Features

- **Three Game Modes:**
  - KOTH (King of the Hill): Teams must hold point for time limit
  - AD (Attack/Defend): Blue attacks, Red defends
  - CP (Control Point): Team with most capture time wins

- **Real-time UI:**
  - Live game clock
  - Team meters (Red/Blue)
  - Seesaw balance meter
  - Control point status with progress
  - Event feed

- **Backend Features:**
  - Bluetooth tag scanning for player/equipment tracking
  - Proximity detection with configurable thresholds
  - Event management with sound triggers
  - Settings persistence
  - Comprehensive test suite

## Hardware Requirements

- Raspberry Pi (3B+ or newer recommended)
- Bluetooth Low Energy tags (advertising at 50ms rate)
- WiFi capability for hosting game network
- Optional: Speaker for sound effects
- Optional: LED strips for visual feedback

## Software Requirements

- Python 3.9 or newer
- See requirements.txt for dependencies

## Installation

### 1. Clone/Download the project

```bash
git clone <repository_url>
cd battlepoint
```

### 2. Create virtual environment

```bash
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
```

### 3. Install dependencies

```bash
pip install -r requirements.txt
```

### 4. Run the application

```bash
python battlepoint_app.py
```

### 5. Access the UI

Open a browser and navigate to:
- Game UI: http://localhost:8080/game
- Settings: http://localhost:8080/settings
- Debug Console: http://localhost:8080/debug

## Configuration

### Game Settings (via Settings UI)

- **Game Mode**: KOTH, AD, or CP
- **Time Limit**: Total game duration (10-600 seconds)
- **Capture Time**: Time to capture point (5-120 seconds)
- **Button Threshold**: Proximity timeout (1-30 seconds)
- **Start Delay**: Countdown before game starts (0-60 seconds)

### Bluetooth Tag Protocol

Tags should advertise with the following binary format:

```
Byte 0: Tag Type (1=Player, 2=Control Square, 3=Health Square)
Byte 1-2: Tag ID (uint16, unique across all tags)
Byte 3: Team (1=RED, 2=BLU, 3=NOBODY)
Bytes 4+: Tag-specific data (reserved for future use)
```

### Service UUID

Set your Bluetooth service UUID in the code:
```python
scanner.service_uuid = "YOUR_SERVICE_UUID_HERE"
```

## API Endpoints

### Game Control
- `POST /api/start` - Start a new game
- `POST /api/stop` - Stop/cancel current game
- `GET /api/state` - Get current game state
- `POST /api/configure` - Update game options

### Proximity Simulation (for testing)
- `POST /api/proximity/red` - Simulate red team proximity
- `POST /api/proximity/blue` - Simulate blue team proximity

### Settings
- `POST /api/settings/save` - Save settings to disk
- `GET /api/settings/load` - Load settings from disk

### Sound (if enabled)
- `POST /api/sound/play/{sound_id}` - Play sound effect
- `POST /api/sound/volume/{volume}` - Set volume (0-30)

### Bluetooth
- `GET /api/bluetooth/tags` - Get active Bluetooth tags
- `POST /api/bluetooth/start` - Start BLE scanning
- `POST /api/bluetooth/stop` - Stop BLE scanning

## Testing

Run the test suite:

```bash
pytest test_battlepoint.py -v
```

Run with coverage:

```bash
pytest test_battlepoint.py --cov=battlepoint_game --cov-report=html
```

## Architecture

### Backend (runs independently)
- **Clock**: Time abstraction (real or test)
- **Proximity**: Tracks player positions via Bluetooth
- **ControlPoint**: Capture logic and state
- **EventManager**: Game events with cooldowns
- **Game**: Base game logic
  - KothGame: King of the Hill implementation
  - ADGame: Attack/Defend implementation
  - CPGame: Control Point implementation
- **BluetoothScanner**: BLE tag detection
- **SoundSystem**: Audio playback
- **SettingsManager**: Persistence

### Frontend (NiceGUI)
- **Game UI** (`/game`): Main game interface
- **Settings UI** (`/settings`): Configuration
- **Debug UI** (`/debug`): Testing and monitoring

### FastAPI
- REST endpoints for backend control
- WebSocket support for real-time updates
- Background task for game loop (100ms update rate)

## Customization

### Adding Sound Effects

1. Place MP3 files in a `sounds/` directory
2. Update the sound map in `SoundSystem.play()`:

```python
sound_map = {
    1: "your_sound.mp3",
    2: "another_sound.mp3",
    # ...
}
```

3. Call sounds via API or event manager

### Adding LED Support

For Raspberry Pi GPIO LEDs:

```python
# Install RPi.GPIO
pip install RPi.GPIO

# In your code:
import RPi.GPIO as GPIO

class LEDController:
    def __init__(self):
        GPIO.setmode(GPIO.BCM)
        self.red_pin = 17
        self.blue_pin = 27
        GPIO.setup(self.red_pin, GPIO.OUT)
        GPIO.setup(self.blue_pin, GPIO.OUT)
    
    def set_red(self, on: bool):
        GPIO.output(self.red_pin, GPIO.HIGH if on else GPIO.LOW)
    
    def set_blue(self, on: bool):
        GPIO.output(self.blue_pin, GPIO.HIGH if on else GPIO.LOW)
```

### Implementing Real Bluetooth Scanning

Replace the stub implementation in `BluetoothScanner`:

```python
from bleak import BleakScanner

async def start_scanning(self):
    self._scanning = True
    
    def detection_callback(device, advertisement_data):
        if self.service_uuid in advertisement_data.service_uuids:
            # Parse manufacturer data or service data
            raw_data = advertisement_data.manufacturer_data.get(0xFFFF)
            if raw_data:
                tag = self.parse_tag_data(bytes(raw_data))
                if tag:
                    self.tags[tag.id] = tag
    
    scanner = BleakScanner(detection_callback=detection_callback)
    await scanner.start()
    
    while self._scanning:
        await asyncio.sleep(0.05)  # 50ms scan rate
    
    await scanner.stop()
```

## Deployment on Raspberry Pi

### 1. Enable Bluetooth

```bash
sudo apt-get update
sudo apt-get install bluetooth bluez python3-bluez
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```

### 2. Set up WiFi Access Point

```bash
sudo apt-get install hostapd dnsmasq
# Configure hostapd.conf and dnsmasq.conf
# See: https://www.raspberrypi.org/documentation/configuration/wireless/access-point.md
```

### 3. Run as service

Create `/etc/systemd/system/battlepoint.service`:

```ini
[Unit]
Description=BattlePoint Game Server
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/battlepoint
ExecStart=/home/pi/battlepoint/venv/bin/python battlepoint_game.py
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl enable battlepoint
sudo systemctl start battlepoint
```

## Troubleshooting

### Bluetooth not working
- Ensure Bluetooth is enabled: `sudo systemctl status bluetooth`
- Check permissions: `sudo usermod -a -G bluetooth $USER`
- Test scanning: `sudo hcitool lescan`

### UI not accessible
- Check firewall: `sudo ufw allow 8080`
- Verify server is running: `sudo systemctl status battlepoint`
- Check logs: `journalctl -u battlepoint -f`

### Game not updating
- Check backend is running: Look for game loop in logs
- Verify proximity updates: Use debug UI to monitor
- Test with manual proximity buttons first

## License

[Your License Here]

## Credits

Based on the Arduino/C++ implementation by dcowden, 2017.
Python/NiceGUI port: 2024.

## Contributing

Pull requests welcome! Please ensure:
- All tests pass
- Code follows PEP 8 style
- New features include tests
- Documentation is updated