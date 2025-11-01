// Adafruit nRF52 "Bluefruit" core (tested with 1.7.0)
// Boards: pdcook:nrfmicro:supermini, adafruit:nrf52:feather52840
#include <bluefruit.h>

// ===== Tunables =====
static const uint16_t ADV_INTERVAL_MS = 40;    // radio advertising interval
static const uint16_t ADV_UPDATE_MS   = 40;    // how often we mutate the payload
static const int8_t   TX_POWER_DBM    = 4;     // -40..+8 typical; clamp if needed
static const uint16_t MFG_COMPANY_ID  = 0xFFFF; // replace with your real company ID

// ===== Rolling state =====
static uint8_t  g_seq          = 0;            // 1-byte rolling counter
static uint32_t g_lastUpdateMs = 0;

// Build a minimal AD structure buffer: [Flags][TxPower][ManufacturerData(companyID + seq)]
// and push it via Bluefruit.Advertising.setData()
static void set_adv_payload(uint8_t seqByte)
{
  uint8_t buf[31];
  uint8_t idx = 0;

  auto put = [&](uint8_t b) {
    if (idx < sizeof(buf)) buf[idx++] = b;
  };

  // AD structure: Flags (type 0x01), value 0x06 (LE General Disc + BR/EDR not supported)
  {
    const uint8_t len = 2;        // length of this AD structure excluding the len byte
    put(len);                     // total bytes after this len: 1(type)+1(data) = 2
    put(0x01);                    // AD Type: Flags
    put(0x06);                    // LE General Discoverable | BR/EDR Not Supported
  }

  // AD structure: Tx Power (type 0x0A)
  {
    const uint8_t len = 2;        // type(1) + value(1)
    put(len);
    put(0x0A);                    // AD Type: Tx Power
    int8_t tx = TX_POWER_DBM;
    if (tx > 8) tx = 8;
    if (tx < -40) tx = -40;
    put((uint8_t)tx);
  }

  // AD structure: Manufacturer Specific Data (type 0xFF)
  // payload: company ID (little-endian) + 1-byte rolling counter
  {
    const uint8_t payloadLen = 2 /*company ID*/ + 1 /*seq*/;
    const uint8_t len = 1 /*type*/ + payloadLen;
    put(len);
    put(0xFF);                    // AD Type: Manufacturer Specific Data
    put((uint8_t)(MFG_COMPANY_ID & 0xFF));       // company ID LSB
    put((uint8_t)((MFG_COMPANY_ID >> 8) & 0xFF)); // company ID MSB
    put(seqByte);                 // our rolling counter
  }

  // Push buffer into the advertising instance. Safe to call while advertising.
  Bluefruit.Advertising.setData(buf, idx);
}

static uint16_t ms_to_units_0p625(uint16_t ms)
{
  if (ms < 20) ms = 20;
  if (ms > 10240) ms = 10240;
  float units = (float)ms / 0.625f;
  return (uint16_t)(units + 0.5f);
}

static void start_advertising()
{
  // Configure the scan response (name lives here; we don't mutate it)
  Bluefruit.ScanResponse.clearData();
  Bluefruit.ScanResponse.addName();

  // Configure advertising parameters
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);

  uint16_t interval_units = ms_to_units_0p625(ADV_INTERVAL_MS);
  Bluefruit.Advertising.setInterval(interval_units, interval_units);

  Bluefruit.Advertising.setFastTimeout(0);          // never drop to slow
  Bluefruit.Advertising.restartOnDisconnect(true);

  // Initial payload with seq=0
  set_adv_payload(g_seq);

  // Start forever
  Bluefruit.Advertising.start(0);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  Bluefruit.begin();
  Bluefruit.setName("PT-789");
  Bluefruit.setTxPower(TX_POWER_DBM);

  start_advertising();
}

void loop()
{
  // heartbeat
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

  // mutate Manufacturer Data periodically to defeat Android duplicate suppression
  uint32_t now = millis();
  if (now - g_lastUpdateMs >= ADV_UPDATE_MS) {
    g_lastUpdateMs = now;
    g_seq++;                      // wraps 255->0
    set_adv_payload(g_seq);
  }

  delay(200);
}
