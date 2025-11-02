// XIAO ESP32-C3 — NimBLE-Arduino (scan loop style, Serial Plotter friendly)

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <map>

// ===== Tunables =====
static const uint16_t ADV_INTERVAL_MS    = 40;
static const uint16_t ADV_UPDATE_MS      = 40;
static const int16_t  RSSI_THRESHOLD_DBM = -60;  // ignore weak ones
static const float    EMA_ALPHA          = 0.20f;
static const uint16_t MFG_COMPANY_ID     = 0xFFFF;

static const int      SCAN_TIME_MS       = 1000;  // drive scan via getResults()

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2 // XIAO ESP32-C3 LED is typically GPIO2
#endif

// Rolling state
static uint8_t  g_seq          = 0;
static uint32_t g_lastUpdateMs = 0;
static volatile uint32_t g_lastHitMs = 0;

struct Ema { bool has=false; float v=0.0f; };
static std::map<String, Ema> g_ema;

static NimBLEScan* pBLEScan = nullptr;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static uint16_t ms_to_units_0p625(uint16_t ms) {
  if (ms < 20) ms = 20;
  if (ms > 10240) ms = 10240;
  return (uint16_t)((float)ms / 0.625f + 0.5f);
}

static void updateAdvPayload(uint8_t seq) {
  NimBLEAdvertisementData advData;
  advData.setFlags(0x06); // LE General Disc. + BR/EDR Not Supported

  // Manufacturer data: 2B company ID (LE) + 1B seq
  std::string mfg;
  mfg.push_back((char)(MFG_COMPANY_ID & 0xFF));        // LSB
  mfg.push_back((char)((MFG_COMPANY_ID >> 8) & 0xFF)); // MSB
  mfg.push_back((char)seq);
  advData.setManufacturerData(mfg);

  // Short name in primary ADV helps with early visibility
  advData.setName("P1");

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->setAdvertisementData(advData);
}

static void startAdvertising() {
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

  const uint16_t units = ms_to_units_0p625(ADV_INTERVAL_MS);
  adv->setMinInterval(units);
  adv->setMaxInterval(units);

  // Keep scan response minimal (optional)
  NimBLEAdvertisementData scanResp;
  adv->setScanResponseData(scanResp);

  updateAdvPayload(g_seq);
  adv->start(); // forever
}

// -----------------------------------------------------------------------------
// Scan callbacks
// -----------------------------------------------------------------------------
class TagScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    int rssi = dev->getRSSI();
    std::string n = dev->getName(); // may be empty until scan response

    // Only track tags named "PT-*"
    if (n.empty() || n.rfind("PT-", 0) != 0) {
      return;
    }

    // Optional: ignore weak
    if (rssi <= RSSI_THRESHOLD_DBM) {
      return;
    }

    String key = String(n.c_str()); // use name as key; swap to MAC if you prefer

    // EMA update (for stable plotting)
    Ema &e = g_ema[key];
    if (!e.has) {
      e.v = (float)rssi;
      e.has = true;
    } else {
      e.v = EMA_ALPHA * (float)rssi + (1.0f - EMA_ALPHA) * e.v;
    }

    // LED latch + timestamp
    digitalWrite(LED_BUILTIN, HIGH);
    g_lastHitMs = millis();

    // -----------------------------------------------------------------
    // Arduino Serial Plotter-friendly output:
    // one line = one scan window (we finish line in onScanEnd)
    // multiple series per line: "<name>:<value> <name2>:<value2> ..."
    // We'll plot the EMA, not raw RSSI, so it's less jumpy.
    // -----------------------------------------------------------------
    Serial.print(">");
    Serial.print(key);
    Serial.print(":");
    Serial.print((int)e.v);    // smoothed RSSI
    Serial.println("");         // space to separate series
  }

  void onScanEnd(NimBLEScanResults) { /* no-op */ }
};

// -----------------------------------------------------------------------------
// Scanning init
// -----------------------------------------------------------------------------
static void initScanning() {
  pBLEScan = NimBLEDevice::getScan();
  static TagScanCallbacks cb;
  pBLEScan->setScanCallbacks(&cb);
  pBLEScan->setActiveScan(true);    // request scan responses (names)
  pBLEScan->setInterval(100);       // ms
  pBLEScan->setWindow(100);         // ms (<= interval)
  pBLEScan->setDuplicateFilter(false);
}

// -----------------------------------------------------------------------------
// Arduino setup/loop
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  NimBLEDevice::init("P1");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // max on this core

  // Start advertising first (kept minimal)
  startAdvertising();

  // Configure scanner like the working sample
  initScanning();

  Serial.println("Setup complete: advertising+scan configured.");
}

void loop() {
  if (pBLEScan) {
    // Blocking scan; calls onResult() for each advertisement
    pBLEScan->getResults(SCAN_TIME_MS, /*clearResults*/ false);

    // We don't print "Devices found: X" anymore — the plotter hates that.
    pBLEScan->clearResults();
  }

  uint32_t now = millis();

  // Periodically mutate manufacturer data
  if (now - g_lastUpdateMs >= ADV_UPDATE_MS) {
    g_lastUpdateMs = now;
    g_seq++;
    updateAdvPayload(g_seq);
  }

  // Auto-off LED if no above-threshold hits for 500 ms
  if ((now - g_lastHitMs) > 500) {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
