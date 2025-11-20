#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>

extern "C" {
  #include "esp_bt.h"   // for btStop() as a hard kill if needed
}

// ============================================================================
// --------------------- BLE CONFIG (was in main) -----------------------------
// ============================================================================


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
static std::string buildManufacturerData_withTeam(const char* tileId,
                                                  char teamChar,
                                                  const char* playerId,
                                                  float strength) {
    std::string m;
    // 2-byte company ID
    m.push_back((char)(MFG_COMPANY_ID & 0xFF));
    m.push_back((char)((MFG_COMPANY_ID >> 8) & 0xFF));

    // clamp strength to a small int for payload size
    int s = (int)(strength + 0.5f);
    if (s < 0) s = 0;
    if (s > 9999) s = 9999;

    char buf[64];
    // TILE,TEAM,PLAYER,STRENGTH  (TEAM is 'R' or 'B' or special code like 'X' for both)
    snprintf(buf, sizeof(buf), "%s,%c,%s,%d",
             (tileId ? tileId : ""),
             teamChar,
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

  BleReporter(const char* tileId)
  : _tileId(tileId ? tileId : ""),
    _state(IDLE),
    _pScan(nullptr),
    _scanStart(0),
    _bestRssi(-999),
    _haveCurrentTag(false),
    _lastAdvUpdate(0),
    _seq(0),
    _lastPlayerPresent(false),
    _currentTeamChar('R'),
    _bleStarted(false) {}



  // Now just initializes internal state; BLE hardware is *not* started here.
  void begin() {
    _state = IDLE;
    _pScan = nullptr;
    _bleStarted = false;
    _lastPlayerPresent = false;
    _haveCurrentTag = false;
    _bestRssi = -999;
    _bestTagName = "";
  }

  // Now takes teamChar so we can encode it in the MFG string
  void update(bool playerPresent, float strength, char teamChar) {
    unsigned long now = millis();
    _currentTeamChar = teamChar;

    // no player → tear down BLE completely
    if (!playerPresent) {
      if (_bleStarted) {
        if (_state == SCANNING && _pScan) {
          _pScan->stop();
          _pScan->clearResults();
        }
        if (_state == ADVERTISING) {
          stopAdvertising();
        }
        shutdownBle();
      }

      _state = IDLE;
      _lastPlayerPresent = false;
      return;
    }

    // rising edge → ensure BLE started, then scan
    if (playerPresent && !_lastPlayerPresent) {
      if (!_bleStarted) {
        startBle();
      }
      startScan(now);
      _lastPlayerPresent = true;
      return;
    }

    switch (_state) {
      case IDLE:
        if (!_bleStarted) {
          startBle();
        }
        Serial.println("Starting Scan");
        startScan(now);
        break;

      case SCANNING: {
        if (now - _scanStart >= SCAN_TIME_MS) {
          if (_pScan) {
            _pScan->stop();

            NimBLEScanResults results = _pScan->getResults();
            pickBestFromResults(results);
            _pScan->clearResults();
          }

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
  std::string _tileId;
  int    _bestRssi;
  String _bestTagName;
  bool   _haveCurrentTag;

  unsigned long _lastAdvUpdate;
  uint8_t _seq;
  bool _lastPlayerPresent;

  char _currentTeamChar; // 'R' or 'B' or special code like 'X' for both
  bool _bleStarted;

  // --------- BLE lifecycle helpers ----------
  void startBle() {
    if (_bleStarted) return;

    Serial.println("BLE: startBle()");
    NimBLEDevice::init(_tileId.c_str());
    NimBLEDevice::setPower(TILE_TX_POWER);

    _pScan = NimBLEDevice::getScan();
    _pScan->setActiveScan(true);
    _pScan->setInterval(100);
    _pScan->setWindow(100);
    _pScan->setDuplicateFilter(false);

    _bleStarted = true;
  }

  void shutdownBle() {
    if (!_bleStarted) return;

    Serial.println("BLE: shutdownBle()");
    // Stop advertising if still running
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (adv && adv->isAdvertising()) {
      adv->stop();
    }

    // Stop scan if still running
    if (_pScan) {
      _pScan->stop();
      _pScan->clearResults();
      _pScan = nullptr;
    }

    // Deinit NimBLE if available; fall back to btStop() if needed
    // If NimBLEDevice::deinit(true) doesn't exist in your version,
    // comment it out and rely on btStop().
    #if defined(NIMBLE_CPP_VERSION) || defined(CONFIG_BT_NIMBLE_ENABLED)
      NimBLEDevice::deinit(true);  // true = release memory
    #endif

    btStop();  // hard-stop BT controller (no-op if already stopped)

    _bleStarted = false;
  }

  // --------- Existing logic, unchanged except for null checks ----------
  void startScan(unsigned long now) {
    if (!_bleStarted || !_pScan) {
      Serial.println("BLE: startScan() called but BLE not started");
      return;
    }

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
    if (!_bleStarted) {
      Serial.println("BLE: startAdvertising() called but BLE not started");
      return;
    }

    Serial.printf("BLE: advertising player %s\n",
                  _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK");

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (!adv) {
      Serial.println("BLE: getAdvertising() returned null");
      return;
    }

    // ADV: just flags + mfg
    NimBLEAdvertisementData advData;
    advData.setFlags(0x04);  // non-connectable, LE only

    std::string mfg = buildManufacturerData_withTeam(
        _tileId.c_str(),
        _currentTeamChar,
        _haveCurrentTag ? _bestTagName.c_str() : "PT-UNK",
        0.0f
    );
    advData.setManufacturerData(mfg);

    // SCAN_RSP: UUID + name
    NimBLEAdvertisementData respData;
    respData.setCompleteServices(NimBLEUUID(SHARED_SERVICE_UUID_STR));
    respData.setName(_tileId.c_str());

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
    if (!_bleStarted) return;
    Serial.println("BLE: stopAdvertising()");
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (adv && adv->isAdvertising()) {
      adv->stop();
    }
  }

  void updateAdvPacket(float strength) {
    if (!_bleStarted) return;

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    if (!adv || !adv->isAdvertising()) return;

    NimBLEAdvertisementData advData;
    advData.setFlags(0x04);

    std::string mfg = buildManufacturerData_withTeam(
        _tileId.c_str(),
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
