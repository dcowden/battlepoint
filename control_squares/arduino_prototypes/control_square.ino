/*
  Control Square v2 (FastLED)
  - QMC5883P: unchanged, same custom read code
  - 15s baseline
  - Presence from fDt vs thresholds
  - On presence: scan BLE for nearest "PT-*" → then advertise “tile X has player Y”
  - LED via FastLED:
      baseline -> OFF
      no player -> RED
      player weak -> YELLOW
      player strong -> GREEN
*/

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// ------------------ BLE ------------------
#include <NimBLEDevice.h>

// ------------------ FastLED ------------------
#include <FastLED.h>

// ============================================================================
// --------------------- USER / HW CONFIG -------------------------------------
// ============================================================================

// LED
#define LED_PIN        8      // change to your actual pin
#define NUM_LEDS       1
#define LED_TYPE       WS2812B
#define LED_COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// Tile identity
static const char* TILE_ID = "SQ-01";  // change per tile

// Shared service UUID that **all** your tags and tiles should use
static const char* SHARED_SERVICE_UUID_STR = "12345678-1234-1234-1234-1234567890ab";

// Manufacturer ID (little-endian in adv)
static const uint16_t MFG_COMPANY_ID = 0xFFFF;

// BLE scan window for presence -> player lookup
static const uint16_t SCAN_TIME_MS = 1000;

// TX power for this tile’s advertising
static const esp_power_level_t TILE_TX_POWER = ESP_PWR_LVL_P6; // or P9

// ============================================================================
// ------------------ QMC5883P (KEEPING YOUR IMPL) -----------------------------
// ============================================================================

const int QMC5883P_ADDR = 0x2C;

// EMA for baseline build
const float BASELINE_ALPHA = 0.05f;
// EMA for plotted output (Dx, Dy, Dz, Dt)
const float OUTPUT_ALPHA   = 0.10f;

// how long to build baseline (15 s)
const unsigned long BASELINE_MS = 15000UL;

// registers
const int MODE_REG   = 0x0A;
const int CONFIG_REG = 0x0B;
const int X_LSB_REG  = 0x01;

// baseline state
float baseX = 0, baseY = 0, baseZ = 0;
bool baselineInit  = false;
bool baselineDone  = false;
unsigned long tStart = 0;

// output EMA state (for Dx/Dy/Dz/Dt)
bool outputEmaInit = false;
float fDx = 0, fDy = 0, fDz = 0, fDt = 0;

// ------------------- presence thresholds (relative) --------------------
const float MIN_ON_DT     = 400.0f;   // enter ON above this
const float STRONG_DT     = 1500.0f;  // GREEN above this
const float OFF_HYST_DT   = 250.0f;   // leave below this

// ------------------------------------------------------------
void initQMC5883P() {
  Wire.begin();
  // continuous, 200 Hz
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(MODE_REG);
  Wire.write(0xCF);
  Wire.endTransmission();

  // set/reset on, ±8G
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(CONFIG_REG);
  Wire.write(0x08);
  Wire.endTransmission();
}

// ------------------------------------------------------------
bool readQMC5883PData(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(X_LSB_REG);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(QMC5883P_ADDR, 6);
  if (Wire.available() != 6) return false;

  byte x_lsb = Wire.read();
  byte x_msb = Wire.read();
  byte y_lsb = Wire.read();
  byte y_msb = Wire.read();
  byte z_lsb = Wire.read();
  byte z_msb = Wire.read();

  x = (x_msb << 8) | x_lsb;
  y = (y_msb << 8) | y_lsb;
  z = (z_msb << 8) | z_lsb;
  return true;
}

float magnitude3D(float x, float y, float z) {
  return sqrtf(x * x + y * y + z * z);
}

// ============================================================================
// ------------------- LED helpers (FastLED) ----------------------------------
// ============================================================================

void led_off() {
  leds[0] = CRGB::Black;
  FastLED.show();
}

void led_red() {
  leds[0] = CRGB(255, 0, 0);
  FastLED.show();
}

void led_yellow() {
  leds[0] = CRGB(255, 180, 0);
  FastLED.show();
}

void led_green() {
  leds[0] = CRGB(0, 180, 0);
  FastLED.show();
}

// ============================================================================
// ------------------- Reporter (BLE) -----------------------------------------
// ============================================================================

class BleReporter {
public:
  enum State {
    IDLE,
    SCANNING,
    ADVERTISING
  };

  BleReporter()
  : _state(IDLE),
    _bestRssi(-999),
    _lastAdvUpdate(0),
    _seq(0),
    _haveCurrentTag(false)
  {}

  void begin() {
    NimBLEDevice::init(TILE_ID);
    NimBLEDevice::setPower(TILE_TX_POWER);
    _pScan = NimBLEDevice::getScan();
    _pScan->setActiveScan(true);
    _pScan->setInterval(100);
    _pScan->setWindow(100);
    _pScan->setDuplicateFilter(false);
  }

  void update(bool playerPresent, float strength /*fDt smoothed*/) {
    unsigned long now = millis();

    if (!playerPresent) {
      if (_state != IDLE) {
        stopAdvertising();
        _state = IDLE;
      }
      return;
    }

    switch (_state) {
      case IDLE:
        startScan();
        _state = SCANNING;
        _scanStart = now;
        break;

      case SCANNING: {
        _pScan->getResults(SCAN_TIME_MS, false);
        pickBestFromResults(_pScan->getResults());
        _pScan->clearResults();

        startAdvertising();
        _state = ADVERTISING;
      } break;

      case ADVERTISING: {
        if (now - _lastAdvUpdate >= 250) {
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
  int _bestRssi;
  String _bestTagName;
  bool _haveCurrentTag;
  unsigned long _lastAdvUpdate;
  uint8_t _seq;

  void startScan() {
    _bestRssi = -999;
    _bestTagName = "";
    _haveCurrentTag = false;
  }

  void pickBestFromResults(const NimBLEScanResults &results) {
    NimBLEUUID svcUUID(SHARED_SERVICE_UUID_STR);

    for (int i = 0; i < results.getCount(); i++) {
      NimBLEAdvertisedDevice dev = results.getDevice(i);
      int rssi = dev.getRSSI();

      std::string nm = dev.getName();
      bool nameOk = (!nm.empty() && nm.rfind("PT-", 0) == 0);
      if (!nameOk) {
        continue;
      }

      bool svcOk = true;
      if (dev.getServiceUUIDCount() > 0) {
        svcOk = dev.isAdvertisingService(svcUUID);
      }
      if (!svcOk) continue;

      if (rssi > _bestRssi) {
        _bestRssi = rssi;
        _bestTagName = String(nm.c_str());
        _haveCurrentTag = true;
      }
    }
  }

  void startAdvertising() {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

    NimBLEAdvertisementData advData;
    advData.setFlags(0x06);
    advData.setCompleteServices(NimBLEUUID(SHARED_SERVICE_UUID_STR));

    std::string mfg = buildManufacturerData(
        _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK",
        0
    );
    advData.setManufacturerData(mfg);
    advData.setName(TILE_ID);

    uint16_t intervalMs = 40;
    uint16_t units = (uint16_t)((float)intervalMs / 0.625f + 0.5f);
    adv->setMinInterval(units);
    adv->setMaxInterval(units);

    NimBLEAdvertisementData scanResp;
    adv->setScanResponseData(scanResp);

    adv->setAdvertisementData(advData);
    adv->start();
  }

  void stopAdvertising() {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    adv->stop();
  }

  std::string buildManufacturerData(const char* playerId, uint8_t strengthByte) {
    std::string m;
    m.push_back((char)(MFG_COMPANY_ID & 0xFF));
    m.push_back((char)((MFG_COMPANY_ID >> 8) & 0xFF));
    m.push_back((char)0x01);      // version
    m.push_back((char)0x01);      // occupied=1
    m.push_back((char)strengthByte);

    uint8_t tileLen = strlen(TILE_ID);
    uint8_t playerLen = strlen(playerId);

    m.push_back((char)tileLen);
    for (uint8_t i = 0; i < tileLen; i++) m.push_back(TILE_ID[i]);

    m.push_back((char)playerLen);
    for (uint8_t i = 0; i < playerLen; i++) m.push_back(playerId[i]);

    if (m.size() > 30) {
      m.resize(30);
    }
    return m;
  }

  void updateAdvPacket(float strength) {
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (!adv->isAdvertising()) return;

    NimBLEAdvertisementData advData;
    advData.setFlags(0x06);
    advData.setCompleteServices(NimBLEUUID(SHARED_SERVICE_UUID_STR));

    float s = strength;
    if (s < 0) s = 0;
    if (s > 2047) s = 2047;
    uint8_t strByte = (uint8_t)(s / 8.0f);

    std::string mfg = buildManufacturerData(
        _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK",
        strByte
    );
    advData.setManufacturerData(mfg);
    advData.setName(TILE_ID);

    adv->setAdvertisementData(advData);
  }
};

// global reporter
BleReporter reporter;

// ============================================================================
// ------------------- GLOBAL PRESENCE STATE ----------------------------------
// ============================================================================

bool g_playerPresent = false;
float g_lastDt = 0.0f;

// ============================================================================
// ------------------- Arduino setup / loop -----------------------------------
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(200);

  FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  led_off();

  initQMC5883P();
  tStart = millis();
  Serial.println("Building 15 s EMA baseline...");

  reporter.begin();
}

void loop() {
  int16_t rx, ry, rz;
  if (!readQMC5883PData(rx, ry, rz)) {
    delay(10);
    return;
  }

  float x = rx;
  float y = ry;
  float z = rz;

  unsigned long now = millis();

  // ================= BASELINE PHASE =================
  if (!baselineDone) {
    if (!baselineInit) {
      baseX = x; baseY = y; baseZ = z;
      baselineInit = true;
    } else {
      baseX = BASELINE_ALPHA * x + (1 - BASELINE_ALPHA) * baseX;
      baseY = BASELINE_ALPHA * y + (1 - BASELINE_ALPHA) * baseY;
      baseZ = BASELINE_ALPHA * z + (1 - BASELINE_ALPHA) * baseZ;
    }

    Serial.println(F(">baseline_building:1"));
    led_off();

    if (now - tStart >= BASELINE_MS) {
      baselineDone = true;
      Serial.println(F("=== Baseline established ==="));
      Serial.print(F("base_x: ")); Serial.println(baseX, 2);
      Serial.print(F("base_y: ")); Serial.println(baseY, 2);
      Serial.print(F("base_z: ")); Serial.println(baseZ, 2);
      Serial.println(F("==========================="));
    }
    delay(10);
    return;
  }

  // ================= RUN PHASE =================
  float dx = x - baseX;
  float dy = y - baseY;
  float dz = z - baseZ;
  float dt = magnitude3D(dx, dy, dz);
  float rawMag = magnitude3D(x, y, z);

  if (!outputEmaInit) {
    fDx = dx;
    fDy = dy;
    fDz = dz;
    fDt = dt;
    outputEmaInit = true;
  } else {
    fDx = OUTPUT_ALPHA * dx + (1 - OUTPUT_ALPHA) * fDx;
    fDy = OUTPUT_ALPHA * dy + (1 - OUTPUT_ALPHA) * fDy;
    fDz = OUTPUT_ALPHA * dz + (1 - OUTPUT_ALPHA) * fDz;
    fDt = OUTPUT_ALPHA * dt + (1 - OUTPUT_ALPHA) * fDt;
  }

  g_lastDt = fDt;

  // Teleplot / debug
  Serial.print(F(">DX:")); Serial.println(fDx, 2);
  Serial.print(F(">DY:")); Serial.println(fDy, 2);
  Serial.print(F(">DZ:")); Serial.println(fDz, 2);
  Serial.print(F(">DT:")); Serial.println(fDt, 3);
  Serial.print(F(">RAW_MAG:")); Serial.println(rawMag, 3);
  Serial.println(F(">baseline_building:0"));

  // presence FSM
  bool prevPresent = g_playerPresent;

  if (!g_playerPresent) {
    if (fDt >= MIN_ON_DT) {
      g_playerPresent = true;
    }
  } else {
    if (fDt <= OFF_HYST_DT) {
      g_playerPresent = false;
    }
  }

  // LED state
  if (!g_playerPresent) {
    // if you really want LED off instead of red, swap to led_off();
    led_red();
  } else {
    if (fDt >= STRONG_DT) {
      led_green();
    } else {
      led_yellow();
    }
  }

  // BLE reporter
  reporter.update(g_playerPresent, fDt);

  delay(10);   // ~100 Hz
}
