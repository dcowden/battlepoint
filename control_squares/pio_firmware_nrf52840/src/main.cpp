#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string>

#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>

// ============================================================================
// --------------------- USER / HW CONFIG -------------------------------------
// ============================================================================

// Set whether positive Z means BLUE (true) or RED (false)
static const bool POSITIVE_Z_IS_BLUE = true;

// Match your ESP32-C3 wiring: NeoPixel on pin 4
#define LED_PIN       4 
#define NUM_LEDS       1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Tile identity
static const char* TILE_ID = "CS-01";

// Shared service UUID (goes in SCAN_RSP)
static const char* SHARED_SERVICE_UUID_STR =
    "12345678-1234-1234-1234-1234567890ab";

// Manufacturer company ID (little-endian)
static const uint16_t MFG_COMPANY_ID = 0xFFFF;

// Scan window when player present
static const uint16_t SCAN_TIME_MS = 400;

// TX power in dBm
static const int8_t TX_POWER_DBM = 8;

// ============================================================================
// ------------------ QMC5883P -------------------------------------------------
const int   QMC5883P_ADDR  = 0x2C;
const float BASELINE_ALPHA = 0.05f;
const float OUTPUT_ALPHA   = 0.10f;
const unsigned long BASELINE_MS = 1000UL;

const int MODE_REG   = 0x0A;
const int CONFIG_REG = 0x0B;
const int X_LSB_REG  = 0x01;

float baseX = 0, baseY = 0, baseZ = 0;
bool  baselineInit  = false;
bool  baselineDone  = false;
unsigned long tStart = 0;

bool  outputEmaInit = false;
float fDx = 0, fDy = 0, fDz = 0, fDt = 0;

// presence thresholds
const float MIN_ON_DT   = 400.0f;
const float STRONG_DT   = 1100.0f;
const float OFF_HYST_DT = 300.0f;

// ============================================================================
// ------------------- LED wrapper --------------------------------------------
class StatusLED {
public:
  StatusLED(uint16_t count)
  : _count(count), _r(0), _g(0), _b(0), _haveColor(false) {}

  void begin(uint8_t brightness = 64) {
    strip.begin();
    strip.setBrightness(brightness);
    off();
  }

  void set(uint8_t r, uint8_t g, uint8_t b) {
    if (!_haveColor || r != _r || g != _g || b != _b) {
      _r = r; _g = g; _b = b;
      _haveColor = true;
      strip.setPixelColor(0, strip.Color(r, g, b));
      strip.show();
      Serial.printf("LED -> %s (%u,%u,%u)\n",
                    colorName(r, g, b), r, g, b);
    }
  }

  void off()    { set(0, 0, 0); }
  void red()    { set(255, 0, 0); }
  void yellow() { set(255, 180, 0); }
  void green()  { set(0, 180, 0); }
  void blue()   { set(0, 0, 255); }

private:
  uint16_t _count;
  uint8_t  _r, _g, _b;
  bool     _haveColor;

  const char* colorName(uint8_t r, uint8_t g, uint8_t b) {
    if (r == 0 && g == 0 && b == 0) return "Black";
    if (r > 220 && g < 60  && b < 60)  return "Red";
    if (g > 160 && r < 80  && b < 80)  return "Green";
    if (b > 160 && r < 80  && g < 80)  return "Blue";
    if (r > 200 && g > 120 && b < 80)  return "Yellow";
    if (r > 200 && b > 200)            return "Magenta";
    if (g > 150 && b > 150)            return "Cyan";
    if (r > 180 && g > 180 && b > 180) return "White";
    return "Custom";
  }
};

StatusLED statusLed(NUM_LEDS);

// ============================================================================
// ------------------- QMC funcs ----------------------------------------------
void initQMC5883P() {
  Serial.println("Starting up QMC5883");
  Wire.setPins(22,24);
  Wire.begin();
  // continuous 200 Hz
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

bool readQMC5883PData(int16_t &x, int16_t &y, int16_t &z) {
  Wire.beginTransmission(QMC5883P_ADDR);
  Wire.write(X_LSB_REG);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(QMC5883P_ADDR, 6);
  if (Wire.available() != 6) return false;

  uint8_t x_lsb = Wire.read();
  uint8_t x_msb = Wire.read();
  uint8_t y_lsb = Wire.read();
  uint8_t y_msb = Wire.read();
  uint8_t z_lsb = Wire.read();
  uint8_t z_msb = Wire.read();

  x = (int16_t)((x_msb << 8) | x_lsb);
  y = (int16_t)((y_msb << 8) | y_lsb);
  z = (int16_t)((z_msb << 8) | z_lsb);
  return true;
}

float magnitude3D(float x, float y, float z) {
  return sqrtf(x * x + y * y + z * z);
}

// ============================================================================
// --------------- MFG builder: "TEAM,TILE,PLAYER,STRENGTH" -------------------
// EXACTLY as in your original ESP32-C3 version.
static std::string buildManufacturerData_withTeam(char teamChar,
                                                  const char* playerId,
                                                  float strength)
{
  std::string m;
  // 2-byte company ID
  m.push_back((char)(MFG_COMPANY_ID & 0xFF));
  m.push_back((char)((MFG_COMPANY_ID >> 8) & 0xFF));

  // clamp strength to keep payload small
  int s = (int)(strength + 0.5f);
  if (s < 0) s = 0;
  if (s > 9999) s = 9999;

  // TEAM,TILE,PLAYER,STRENGTH
  char buf[64];
  snprintf(buf, sizeof(buf), "%c,%s,%s,%d",
           teamChar,
           TILE_ID,
           playerId,
           s);

  m.append(buf);
  return m;
}

// ============================================================================
// ------------------- BLE Reporter (Bluefruit) -------------------------------
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
      _haveCurrentTag(false),
      _lastAdvUpdate(0),
      _seq(0),
      _lastPlayerPresent(false),
      _currentTeamChar('R'),
      _scanStart(0)
  {}

  void begin() {
    instance = this;

    // Enable 1 peripheral + 1 central (scanner)
    Bluefruit.begin(1, 1);
    Bluefruit.setName(TILE_ID);
    Bluefruit.setTxPower(TX_POWER_DBM);
    //Bluefruit.autoConnLed(false); // don't steal our LED pin

    // Scan Response: static (Name + shared UUID)
    Bluefruit.ScanResponse.clearData();
    Bluefruit.ScanResponse.addName();
    BLEUuid svcUuid(SHARED_SERVICE_UUID_STR);
    Bluefruit.ScanResponse.addUuid(svcUuid);

    // Scanner setup (just harvest PLB/PLR names)
    Bluefruit.Scanner.setRxCallback(scanCallbackThunk);
    Bluefruit.Scanner.setInterval(160, 80); // 100 ms / 50 ms
    Bluefruit.Scanner.useActiveScan(true);
  }

  void update(bool playerPresent, float strength, char teamChar) {
    unsigned long now = millis();
    _currentTeamChar = teamChar;

    if (!playerPresent) {
      if (_state == SCANNING) {
        Bluefruit.Scanner.stop();
      }
      if (_state == ADVERTISING) {
        Bluefruit.Advertising.stop();
      }
      _state = IDLE;
      _lastPlayerPresent = false;
      return;
    }

    // rising edge → start scan
    if (playerPresent && !_lastPlayerPresent) {
      startScan(now);
      _lastPlayerPresent = true;
      return;
    }

    switch (_state) {
      case IDLE:
        startScan(now);
        break;

      case SCANNING:
        if (now - _scanStart >= SCAN_TIME_MS) {
          Bluefruit.Scanner.stop();
          startAdvertising();   // uses best tag (if any) + teamChar
          _state = ADVERTISING;
        }
        break;

      case ADVERTISING:
        if (now - _lastAdvUpdate >= 100) {
          _lastAdvUpdate = now;
          _seq++;
          updateAdvPacket(strength);
        }
        break;
    }
  }

private:
  State _state;

  int    _bestRssi;
  String _bestTagName;
  bool   _haveCurrentTag;

  unsigned long _lastAdvUpdate;
  uint8_t _seq;
  bool _lastPlayerPresent;

  char _currentTeamChar;
  unsigned long _scanStart;

  static BleReporter* instance;

  // ===== Scanner glue =======================================================
  static void scanCallbackThunk(ble_gap_evt_adv_report_t* report) {
    if (instance && report) {
      instance->handleScanReport(report);
    }
  }

  void handleScanReport(ble_gap_evt_adv_report_t* report) {
    char nameBuf[32] = {0};

    uint8_t len = Bluefruit.Scanner.parseReportByType(
        report,
        BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
        (uint8_t*)nameBuf,
        sizeof(nameBuf) - 1);

    if (len == 0) {
      len = Bluefruit.Scanner.parseReportByType(
          report,
          BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME,
          (uint8_t*)nameBuf,
          sizeof(nameBuf) - 1);
    }
    nameBuf[len] = 0;

    const char* nm = (len > 0) ? nameBuf : "(no name)";
    int rssi = report->rssi;

    // Deterministic “best PLB/PLR” selection like your NimBLE version
    pickBestFromReport(nm, rssi);
  }

  void pickBestFromReport(const char* name, int rssi) {
    std::string nm = (name && name[0]) ? name : "(no name)";
    Serial.printf("%s: %d\n", nm.c_str(), rssi);

    bool nameOk =
      (nm.find("PLB") != std::string::npos) ||
      (nm.find("PLR") != std::string::npos);

    if (!nameOk) return;

    if (rssi > _bestRssi) {
      _bestRssi = rssi;
      _bestTagName = String(nm.c_str());
      _haveCurrentTag = true;
    }
  }

  void startScan(unsigned long now) {
    Serial.println("BLE: startScan()");
    _bestRssi = -999;
    _bestTagName = "";
    _haveCurrentTag = false;

    Bluefruit.Scanner.start(0); // run until we stop after SCAN_TIME_MS
    _scanStart = now;
    _state = SCANNING;
  }

  // ===== Advertising helpers ===============================================

  static uint16_t ms_to_units_0p625(uint16_t ms) {
    if (ms < 20) ms = 20;
    if (ms > 10240) ms = 10240;
    float units = (float)ms / 0.625f;
    return (uint16_t)(units + 0.5f);
  }


void setManufacturerString(const char* str) {
  size_t len = strlen(str);
  size_t total_len = 2 + len;  // 2 bytes for company ID

  // Safety guard: BLE advertising payload max is 31 bytes
  if (total_len > 31) len = 29;  // leave room for header bytes

  uint8_t buf[31];
  buf[0] = (uint8_t)(MFG_COMPANY_ID & 0xFF);
  buf[1] = (uint8_t)(MFG_COMPANY_ID >> 8);
  memcpy(buf + 2, str, len);

  // Reset the advertising payload if you want a fresh set
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addManufacturerData(buf, 2 + len);

}

  // Build ADV payload via official helpers:
  //  - Flags: BR/EDR not supported, LE only (0x04)
  //  - Tx Power
  //  - Manufacturer Data: 0xFF + [0xFFFF + TEAM,TILE,PLAYER,STRENGTH]
  void applyAdvPayload() {
    Bluefruit.Advertising.stop();
    const char B = 'B';
    std::string mfg = buildManufacturerData_withTeam(
        _currentTeamChar,
        &B, 
        2000.0
        );
    Serial.printf("BLE: advertising player , mfg=%s\n" ,mfg.c_str());        
    setManufacturerString(mfg.c_str());
    Bluefruit.Advertising.start();
  }

  void startAdvertising() {
    const char* playerId =
      _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK";

  
    Bluefruit.Advertising.setType(
        BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);

    uint16_t itvl = ms_to_units_0p625(40);
    Bluefruit.Advertising.setInterval(itvl, itvl);
    Bluefruit.Advertising.setFastTimeout(0);
    Bluefruit.Advertising.restartOnDisconnect(true);

    applyAdvPayload();

    // Name + UUID are in ScanResponse only (set in begin())
    Bluefruit.Advertising.start(0);

    _lastAdvUpdate = millis();
  }

  void updateAdvPacket(float strength) {
    if (!Bluefruit.Advertising.isRunning()) return;

    const char* playerId =
      _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK";

    applyAdvPayload();
  }
};

BleReporter* BleReporter::instance = nullptr;
BleReporter reporter;

// ============================================================================
// ------------------- GLOBAL PRESENCE STATE ----------------------------------
bool  g_playerPresent = false;
float g_lastDt = 0.0f;

// Helper: compute team char from filtered Z sign
static inline char teamFromZ(float filteredDz) {
  bool zPos = (filteredDz >= 0.0f);
  if (POSITIVE_Z_IS_BLUE) {
    return zPos ? 'B' : 'R';
  } else {
    return zPos ? 'R' : 'B';
  }
}

// ============================================================================
// ------------------- Arduino setup / loop -----------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("Startup...");
  delay(200);

  statusLed.begin(64);
  statusLed.off();

  initQMC5883P();
  tStart = millis();
  Serial.println("Building baseline...");

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

  // baseline phase
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
    statusLed.off();

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

  // run phase
  float dx = x - baseX;
  float dy = y - baseY;
  float dz = z - baseZ;
  float dt = magnitude3D(dx, dy, dz);

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

  //Serial.printf(">fDt: %0.1f\n", fDt);
  g_lastDt = fDt;

  // presence FSM
  if (!g_playerPresent) {
    if (fDt >= MIN_ON_DT) {
      g_playerPresent = true;
    }
  } else {
    if (fDt <= OFF_HYST_DT) {
      g_playerPresent = false;
    }
  }

  // LED state (unchanged logic)
  if (!g_playerPresent) {
    statusLed.red();
  } else {
    if (fDt >= STRONG_DT) {
      statusLed.green();
    } else {
      statusLed.yellow();
    }
  }

  // team from magnet
  char teamChar = 'R';
  if (g_playerPresent) {
    teamChar = teamFromZ(fDz);
    if (teamChar == 'R') {
      //Serial.println(">P: 1");
    } else if (teamChar == 'B') {
      //Serial.println(">P: 2");
    }
  } else {
    //Serial.println(">P: 0");
  }

  // BLE FSM
  reporter.update(g_playerPresent, fDt, teamChar);

  delay(10); // ~100 Hz
}
