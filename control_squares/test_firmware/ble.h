#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>

// ============================================================================
// --------------------- BLE CONFIG (was in main) -----------------------------
// ============================================================================

// your tile name
static const char* TILE_ID = "CS-02";

// shared service UUID (goes in SCAN_RSP, not ADV)
static const char* SHARED_SERVICE_UUID_STR = "12345678-1234-1234-1234-1234567890ab";

// manufacturer to stick at the front (little-endian)
static const uint16_t MFG_COMPANY_ID = 0xFFFF;

// scan window when a player is present
static const uint16_t SCAN_TIME_MS = 100;

// TX power
static const esp_power_level_t TILE_TX_POWER = ESP_PWR_LVL_P9;

// ============================================================================
// --------------- MFG builder: "TEAM,TILE,PLAYER,STRENGTH" -------------------
static std::string buildManufacturerData_withTeam(char teamChar, const char* playerId, float strength) {
    std::string m;
    // 2-byte company ID
    m.push_back((char)(MFG_COMPANY_ID & 0xFF));
    m.push_back((char)((MFG_COMPANY_ID >> 8) & 0xFF));

    // clamp strength to a small int for payload size
    int s = (int)(strength + 0.5f);
    if (s < 0) s = 0;
    if (s > 9999) s = 9999;

    char buf[64];
    // TEAM,TILE,PLAYER,STRENGTH  (TEAM is 'R' or 'B' or special code like 'X' for both)
    snprintf(buf, sizeof(buf), "%c,%s,%s,%d",
             teamChar,
             TILE_ID,
             playerId,
             s);

    m.append(buf);
    return m;
}

// ============================================================================
// ------------------- Reporter (BLE) -----------------------------------------
class BleReporter {
public:
  enum State {
    IDLE,
    SCANNING,
    ADVERTISING
  };

  BleReporter()
  : _state(IDLE),
    _pScan(nullptr),
    _scanStart(0),
    _bestRssi(-999),
    _haveCurrentTag(false),
    _lastAdvUpdate(0),
    _seq(0),
    _lastPlayerPresent(false),
    _currentTeamChar('R') {}

  void begin() {
    NimBLEDevice::init(TILE_ID);
    NimBLEDevice::setPower(TILE_TX_POWER);

    _pScan = NimBLEDevice::getScan();
    _pScan->setActiveScan(true);
    _pScan->setInterval(100);
    _pScan->setWindow(100);
    _pScan->setDuplicateFilter(false);
  }

  // Now takes teamChar so we can encode it in the MFG string
  void update(bool playerPresent, float strength, char teamChar) {
    unsigned long now = millis();
    _currentTeamChar = teamChar;

    // no player → tear down
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

    // rising edge → scan
    if (playerPresent && !_lastPlayerPresent) {
      startScan(now);
      _lastPlayerPresent = true;
      return;
    }

    switch (_state) {
      case IDLE:
        Serial.println("Starting Scan");
        startScan(now);
        break;

      case SCANNING: {
        if (now - _scanStart >= SCAN_TIME_MS) {
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
          updateAdvPacket(strength);   // includes _currentTeamChar
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

  char _currentTeamChar; // 'R' or 'B' or special code like 'X' for both

  void startScan(unsigned long now) {
    Serial.println("BLE: startScan()");
    _bestRssi = -999;
    _bestTagName = "";
    _haveCurrentTag = false;

    _pScan->start(0, false);
    _scanStart = now;
    _state = SCANNING;
  }

  void pickBestFromResults(const NimBLEScanResults &results) {
    Serial.println("BLE: selecting best PLB*/PLR* (anywhere in name)");
    _bestRssi = -999;
    _bestTagName = "";
    _haveCurrentTag = false;

    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice* dev = results.getDevice(i);
      if (!dev) continue;

      int rssi = dev->getRSSI();
      std::string nm = dev->getName();
      if (nm.empty()) nm = "(no name)";

      Serial.printf("%s: %d\n", nm.c_str(), rssi);

      bool nameOk = (nm.find("PLB") != std::string::npos ||
                     nm.find("PLR") != std::string::npos);
      if (!nameOk) continue;

      if (rssi > _bestRssi) {
        _bestRssi = rssi;
        _bestTagName = String(nm.c_str());
        _haveCurrentTag = true;
      }
    }

    if (_haveCurrentTag) {
      Serial.printf("Best tag: %s (%d dBm)\n", _bestTagName.c_str(), _bestRssi);
    } else {
      Serial.println("No PLB*/PLR* tags found");
    }
  }

  void startAdvertising() {
    Serial.printf("BLE: advertising player %s\n",
                  _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK");

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

    // ADV: just flags + mfg
    NimBLEAdvertisementData advData;
    advData.setFlags(0x04);  // non-connectable, LE only

    std::string mfg = buildManufacturerData_withTeam(
        _currentTeamChar,
        _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK",
        0.0f                    // first packet, 0 strength
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
    Serial.println("BLE: stopAdvertising()");
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

    // leave scan response alone
    adv->setAdvertisementData(advData);
  }
};

// global reporter is defined in main.cpp
extern BleReporter reporter;

#endif // BLE_H
