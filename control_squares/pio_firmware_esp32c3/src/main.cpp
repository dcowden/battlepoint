#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string>

// ------------------ BLE ------------------
#include <NimBLEDevice.h>

// ------------------ NeoPixel -------------
#include <Adafruit_NeoPixel.h>

// ------------------ Debounce -------------
#include <Bounce2.h>

// ============================================================================
// --------------------- USER / HW CONFIG -------------------------------------
// ============================================================================

// NOTE: on XIAO ESP32-C3 SDA/SCL are 4/5.
// If you actually use I2C on 4/5, move the LED to another pin.
#define LED_PIN        4
#define NUM_LEDS       1

// Debounced, active-LOW inputs: LOW means “present”
#define BLUE_PIN       6     // pick your real pins
#define RED_PIN        7

// Tile identity
static const char* TILE_ID = "CS-02";

// shared service UUID (goes in SCAN_RSP, not ADV)
static const char* SHARED_SERVICE_UUID_STR = "12345678-1234-1234-1234-1234567890ab";

// manufacturer to stick at the front (little-endian)
static const uint16_t MFG_COMPANY_ID = 0xFFFF;

// TX power
static const esp_power_level_t TILE_TX_POWER = ESP_PWR_LVL_P9;

// Sleep behavior
static const uint64_t READY_WAKE_US       = 50ULL * 1000ULL;  // 50 ms
static const uint32_t READY_SLEEP_DELAYMS = 30000UL;          // 30 s no-people before resuming sleep

// ============================================================================
// ------------------ QMC5883P -------------------------------------------------
const uint8_t    QMC5883P_ADDR  = 0x2C;
const float BASELINE_ALPHA = 0.05f;
const float OUTPUT_ALPHA   = 0.10f;
const unsigned long BASELINE_MS = 10000UL;

const int MODE_REG   = 0x0A;
const int CONFIG_REG = 0x0B;
const int X_LSB_REG  = 0x01;

float baseX = 0, baseY = 0, baseZ = 0;
bool  baselineInit  = false;
bool  baselineDone  = false;
unsigned long tStart = 0;

bool  outputEmaInit = false;
float fDx = 0, fDy = 0, fDz = 0, fDt = 0;

// presence thresholds (magnetic fallback; pins are authoritative)
const float MIN_ON_DT   = 300.0f;
const float STRONG_DT   = 1100.0f;
const float OFF_HYST_DT = 300.0f;

// ============================================================================
// ------------------- tiny SoftTicker (sleep-proof) --------------------------
class SoftTicker {
public:
  SoftTicker() : _period(1000), _last(0), _enabled(false) {}
  void setPeriod(uint32_t ms) { _period = ms; }
  void start() { _last = millis(); _enabled = true; }
  void stop() { _enabled = false; }
  bool ready() {
    if (!_enabled) return false;
    uint32_t now = millis();
    if ((uint32_t)(now - _last) >= _period) {
      _last = now;
      return true;
    }
    return false;
  }
private:
  uint32_t _period, _last;
  bool _enabled;
};

// ============================================================================
// ------------------- LED wrapper --------------------------------------------
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

class StatusLED {
public:
  StatusLED(uint16_t count) : _count(count), _r(0), _g(0), _b(0), _haveColor(false), _blink(false), _blinkOnMs(250), _blinkOffMs(750), _blinkState(false), _lastToggle(0) {}

  void begin(uint8_t brightness = 64) {
    strip.begin();
    strip.setBrightness(brightness);
    off();
  }

  void set(uint8_t r, uint8_t g, uint8_t b) {
    _blink = false;
    push(r, g, b);
  }

  void off() { set(0, 0, 0); }
  void red() { set(255, 0, 0); }
  void green() { set(0, 180, 0); }
  void blue() { set(0, 0, 255); }
  void yellow() { set(255, 180, 0); }

  // sleep-proof blinker (driven from loop())
  void blink(uint8_t r, uint8_t g, uint8_t b, uint32_t onMs, uint32_t offMs) {
    _blink = true;
    _blinkColor[0] = r; _blinkColor[1] = g; _blinkColor[2] = b;
    _blinkOnMs = onMs;
    _blinkOffMs = offMs;
    // do not block: toggle by millis in service()
  }

  void service() {
    if (!_blink) return;
    uint32_t now = millis();
    uint32_t dur = _blinkState ? _blinkOnMs : _blinkOffMs;
    if ((uint32_t)(now - _lastToggle) >= dur) {
      _lastToggle = now;
      _blinkState = !_blinkState;
      if (_blinkState) {
        push(_blinkColor[0], _blinkColor[1], _blinkColor[2]);
      } else {
        push(0,0,0);
      }
    }
  }

private:
  uint16_t _count;
  uint8_t  _r, _g, _b;
  bool     _haveColor;

  bool _blink;
  uint8_t _blinkColor[3];
  uint32_t _blinkOnMs, _blinkOffMs;
  bool _blinkState;
  uint32_t _lastToggle;

  void push(uint8_t r, uint8_t g, uint8_t b) {
    if (!_haveColor || r != _r || g != _g || b != _b) {
      _r = r; _g = g; _b = b;
      _haveColor = true;
      strip.setPixelColor(0, strip.Color(r, g, b));
      strip.show();
    }
  }
};

StatusLED statusLed(NUM_LEDS);

// ============================================================================
// ------------------- QMC funcs ----------------------------------------------
static uint8_t qmc_err_streak = 0;

void initQMC5883P() {
  Wire.begin();
  Wire.setTimeOut(5); // ms — prevents lockup
  // continuous 200 Hz
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(MODE_REG);
  Wire.write(0xCF);
  Wire.endTransmission(true);

  // set/reset on, ±8G
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(CONFIG_REG);
  Wire.write(0x08);
  Wire.endTransmission(true);

  qmc_err_streak = 0;
}

bool readQMC5883PData(int16_t &x, int16_t &y, int16_t &z) {
  // position register pointer
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(X_LSB_REG);
  if (Wire.endTransmission(false) != 0) {
    if (++qmc_err_streak >= 3) { initQMC5883P(); }
    return false;
  }

  // Explicit types so we hit requestFrom(uint16_t, uint8_t, uint8_t)
  uint8_t req = Wire.requestFrom((uint16_t)QMC5883P_ADDR,
                                 (uint8_t)6,
                                 (uint8_t)true);
  if (req != 6) {
    if (++qmc_err_streak >= 3) { initQMC5883P(); }
    return false;
  }

  if (Wire.available() < 6) {
    if (++qmc_err_streak >= 3) { initQMC5883P(); }
    return false;
  }

  uint8_t x_lsb = Wire.read();
  uint8_t x_msb = Wire.read();
  uint8_t y_lsb = Wire.read();
  uint8_t y_msb = Wire.read();
  uint8_t z_lsb = Wire.read();
  uint8_t z_msb = Wire.read();

  x = (int16_t)((x_msb << 8) | x_lsb);
  y = (int16_t)((y_msb << 8) | y_lsb);
  z = (int16_t)((z_msb << 8) | z_lsb);

  qmc_err_streak = 0;
  return true;
}


float magnitude3D(float x, float y, float z) {
  return sqrtf(x * x + y * y + z * z);
}

// ============================================================================
// --------------- MFG builder: "TEAM,TILE,PLAYER,STRENGTH" -------------------
static std::string buildManufacturerData_withTeam(char teamChar, const char* playerId, float strength) {
  std::string m;
  // 2-byte company ID
  m.push_back((char)(MFG_COMPANY_ID & 0xFF));
  m.push_back((char)((MFG_COMPANY_ID >> 8) & 0xFF));

  int s = (int)(strength + 0.5f);
  if (s < 0) s = 0;
  if (s > 9999) s = 9999;

  char buf[64];
  snprintf(buf, sizeof(buf), "%c,%s,%s,%d", teamChar, TILE_ID,
           playerId ? playerId : "PT-UNK", s);

  m.append(buf);
  return m;
}

// ============================================================================
// ------------------- Reporter (BLE) -----------------------------------------
class BleReporter {
public:
  enum State { IDLE, SCANNING, ADVERTISING };

  BleReporter()
  : _state(IDLE), _pScan(nullptr), _scanStart(0), _bestRssi(-999),
    _haveCurrentTag(false), _lastAdvUpdate(0), _seq(0),
    _lastPlayerPresent(false), _currentTeamChar('R') {}

  void begin() {
    NimBLEDevice::init(TILE_ID);
    NimBLEDevice::setPower(TILE_TX_POWER);

    _pScan = NimBLEDevice::getScan();
    _pScan->setActiveScan(true);
    _pScan->setInterval(100);
    _pScan->setWindow(100);
    _pScan->setDuplicateFilter(false);
  }

  // teamChar comes from the debounced pins if available; otherwise fallback
  void update(bool playerPresent, float strength, char teamChar) {
    unsigned long now = millis();
    _currentTeamChar = teamChar;

    if (!playerPresent) {
      if (_state == SCANNING) {
        _pScan->stop();
        _pScan->clearResults();
      }
      if (_state == ADVERTISING) {
        stopAdvertising();
      }
      _state = IDLE;
      _lastPlayerPresent = false;
      return;
    }

    if (playerPresent && !_lastPlayerPresent) {
      startScan(now);
      _lastPlayerPresent = true;
      return;
    }

    switch (_state) {
      case IDLE:
        startScan(now);
        break;

      case SCANNING: {
        if (now - _scanStart >= 400 /*ms*/) {
          _pScan->stop();

          NimBLEScanResults results = _pScan->getResults();
          pickBestFromResults(results);
          _pScan->clearResults();

          startAdvertising(); // uses _currentTeamChar for the first packet
          _state = ADVERTISING;
        }
      } break;

      case ADVERTISING: {
        if (now - _lastAdvUpdate >= 100) {
          _lastAdvUpdate = now;
          _seq++;
          updateAdvPacket(strength);
        }
      } break;
    }
  }

private:
  State _state;
  NimBLEScan* _pScan;
  unsigned long _scanStart;

  int    _bestRssi;
  String _bestTagName;
  bool   _haveCurrentTag;

  unsigned long _lastAdvUpdate;
  uint8_t _seq;
  bool _lastPlayerPresent;

  char _currentTeamChar; // 'R' or 'B'

  void startScan(unsigned long now) {
    _bestRssi = -999;
    _bestTagName = "";
    _haveCurrentTag = false;

    _pScan->start(0, false);
    _scanStart = now;
    _state = SCANNING;
  }

  void pickBestFromResults(const NimBLEScanResults &results) {
    _bestRssi = -999;
    _bestTagName = "";
    _haveCurrentTag = false;

    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice* dev = results.getDevice(i);
      if (!dev) continue;

      int rssi = dev->getRSSI();
      std::string nm = dev->getName();
      if (nm.empty()) nm = "(no name)";

      bool nameOk = (nm.find("PLB") != std::string::npos ||
                     nm.find("PLR") != std::string::npos);
      if (!nameOk) continue;

      if (rssi > _bestRssi) {
        _bestRssi = rssi;
        _bestTagName = String(nm.c_str());
        _haveCurrentTag = true;
      }
    }
  }

  void startAdvertising() {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

    // ADV: just flags + mfg
    NimBLEAdvertisementData advData;
    advData.setFlags(0x04);  // non-connectable, LE only

    std::string mfg = buildManufacturerData_withTeam(
        _currentTeamChar,
        _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK",
        0.0f
    );
    advData.setManufacturerData(mfg);

    // SCAN_RSP: UUID + name
    NimBLEAdvertisementData respData;
    respData.setCompleteServices(NimBLEUUID(SHARED_SERVICE_UUID_STR));
    respData.setName(TILE_ID);

    uint16_t intervalMs = 40;
    uint16_t units = (uint16_t)((float)intervalMs / 0.625f + 0.5f);
    adv->setMinInterval(units);
    adv->setMaxInterval(units);

    adv->setAdvertisementData(advData);
    adv->setScanResponseData(respData);

    adv->start();
    _lastAdvUpdate = millis();
  }

  void stopAdvertising() {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    adv->stop();
  }

  void updateAdvPacket(float strength) {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (!adv->isAdvertising()) return;

    NimBLEAdvertisementData advData;
    advData.setFlags(0x04);

    std::string mfg = buildManufacturerData_withTeam(
        _currentTeamChar,
        _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK",
        strength
    );
    advData.setManufacturerData(mfg);
    adv->setAdvertisementData(advData);
  }
};

BleReporter reporter;

// ============================================================================
// ------------------- GLOBAL STATE / HELPERS ---------------------------------
Bounce blueDebouncer;
Bounce redDebouncer;

bool  g_playerPresent = false;  // aggregate “someone on” view
float g_lastDt = 0.0f;          // magnetic strength (fallback/telemetry)

enum RunState { RS_BASELINING, RS_READY, RS_PERSON };
RunState g_state = RS_BASELINING;

uint32_t ready_since_ms = 0;    // when we entered READY (for 30s rule)
bool     allow_sleep = false;   // only true if READY for >= 30s

static inline char teamFromPinsOrMag(bool blueOn, bool redOn, float filteredDz) {
  if (blueOn && !redOn) return 'B';
  if (redOn && !blueOn) return 'R';
  // both or neither -> fall back to magnet sign for team hint
  return (filteredDz >= 0.0f) ? 'B' : 'R';
}

// LED blinker controller (same timing for baseline & ready)
static const uint32_t BLINK_ON  = 250;
static const uint32_t BLINK_OFF = 750;

// ============================================================================
// ------------------- Arduino setup / loop -----------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(BLUE_PIN, INPUT_PULLUP);
  pinMode(RED_PIN,  INPUT_PULLUP);

  blueDebouncer.attach(BLUE_PIN, INPUT_PULLUP);
  blueDebouncer.interval(10);
  redDebouncer.attach(RED_PIN, INPUT_PULLUP);
  redDebouncer.interval(10);

  statusLed.begin(64);
  statusLed.off();

  initQMC5883P();
  tStart = millis();
  g_state = RS_BASELINING;

  reporter.begin();

  // Sleep source (timer) is configured ad-hoc when used; nothing to do here.
}

void enterReady() {
  g_state = RS_READY;
  ready_since_ms = millis();
  allow_sleep = false; // 30s rule
  // Ready LED: green fast blink
  statusLed.blink(0, 180, 0, BLINK_ON, BLINK_OFF);
}

void enterPerson() {
  g_state = RS_PERSON;
  allow_sleep = false; // awake while person on
}

void manageSleep() {
  if (g_state == RS_READY) {
    // Decide if 30s of continuous READY has elapsed
    uint32_t now = millis();
    if (!allow_sleep && (uint32_t)(now - ready_since_ms) >= READY_SLEEP_DELAYMS) {
      allow_sleep = true;
    }
    if (allow_sleep) {
      // Sleep 50ms between polls
      esp_sleep_enable_timer_wakeup(READY_WAKE_US);
      esp_light_sleep_start(); // returns after wake
    }
  }
}

void loop() {
  // -------- Debounced pins
  blueDebouncer.update();
  redDebouncer.update();
  bool blueOn = (blueDebouncer.read() == LOW);
  bool redOn  = (redDebouncer.read()  == LOW);

  // -------- Sensor read (non-blocking; if it fails, we still progress)
  int16_t rx, ry, rz;
  bool got = readQMC5883PData(rx, ry, rz);
  float x=0, y=0, z=0;
  if (got) { x = rx; y = ry; z = rz; }

  unsigned long now = millis();

  // -------- Baseline phase (runs even if some reads fail)
  if (g_state == RS_BASELINING) {
    // LED: fast blink (distinct while calibrating)
    statusLed.blink(255, 180, 0, BLINK_ON, BLINK_OFF); // yellow fast blink

    if (got) {
      if (!baselineInit) {
        baseX = x; baseY = y; baseZ = z;
        baselineInit = true;
      } else {
        baseX = BASELINE_ALPHA * x + (1 - BASELINE_ALPHA) * baseX;
        baseY = BASELINE_ALPHA * y + (1 - BASELINE_ALPHA) * baseY;
        baseZ = BASELINE_ALPHA * z + (1 - BASELINE_ALPHA) * baseZ;
      }
    }
    if (now - tStart >= BASELINE_MS) {
      baselineDone = true;
      outputEmaInit = false;
      enterReady();
    }
    statusLed.service(); // drive blink
    delay(1);
    return;
  }

  // -------- Run phase: EMA + magnitude (tolerate missed reads)
  float dx=0, dy=0, dz=0, dt=0;
  if (got) {
    dx = x - baseX;
    dy = y - baseY;
    dz = z - baseZ;
    dt = magnitude3D(dx, dy, dz);

    if (!outputEmaInit) {
      fDx = dx; fDy = dy; fDz = dz; fDt = dt;
      outputEmaInit = true;
    } else {
      fDx = OUTPUT_ALPHA * dx + (1 - OUTPUT_ALPHA) * fDx;
      fDy = OUTPUT_ALPHA * dy + (1 - OUTPUT_ALPHA) * fDy;
      fDz = OUTPUT_ALPHA * dz + (1 - OUTPUT_ALPHA) * fDz;
      fDt = OUTPUT_ALPHA * dt + (1 - OUTPUT_ALPHA) * fDt;
    }
    g_lastDt = fDt;
  }

  // -------- Determine presence
  // Pins are authoritative: any active pin = person present
  bool pinPresent = (blueOn || redOn);

  // Magnetic fallback (only if no pins active), with hysteresis
  static bool magPresent = false;
  if (!pinPresent && outputEmaInit) {
    if (!magPresent) {
      if (fDt >= MIN_ON_DT) magPresent = true;
    } else {
      if (fDt <= OFF_HYST_DT) magPresent = false;
    }
  } else if (pinPresent) {
    magPresent = false; // irrelevant while pins asserted
  }

  bool someonePresent = pinPresent || magPresent;

  // -------- State transitions
  if (g_state == RS_READY) {
    if (someonePresent) {
      enterPerson();
    }
  } else if (g_state == RS_PERSON) {
    if (!someonePresent) {
      enterReady(); // resets ready_since timer
    }
  }

  // -------- LED according to spec
  if (g_state == RS_READY) {
    // green fast blink
    statusLed.blink(0, 180, 0, BLINK_ON, BLINK_OFF);
  } else if (g_state == RS_PERSON) {
    // Solid based on team (pins take priority; else magnet sign)
    char teamChar = teamFromPinsOrMag(blueOn, redOn, fDz);
    if (teamChar == 'B') statusLed.blue();
    else                 statusLed.red();
  }
  statusLed.service(); // tick blinker if active

  // -------- Team for advertising
  char advTeam = teamFromPinsOrMag(blueOn, redOn, fDz);

  // -------- BLE reporter
  reporter.update(someonePresent, g_lastDt, advTeam);

  // -------- Power: sleep only in READY
  manageSleep();

  // Small pacing; tolerant with light-sleep cadence
  delay(1);
}
