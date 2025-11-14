#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string>


#define COLOR_RED     255,0,0
#define COLOR_BLUE    0,0,255
#define COLOR_YELLOW  255,255,0
#define COLOR_PURPLE  128,0,128
#define COLOR_GREEN   0, 180, 0
#define COLOR_GRAY   100, 100, 100


// ------------------ NeoPixel -------------
#include <Adafruit_NeoPixel.h>

// ------------------ QMC auto-detect ------
#include "qmc5883.h"

#include "ble.h"
#include "led.h"   // our LED wrapper

// ------------------ Debounce -------------
#include <Bounce2.h>

// ============================================================================
// --------------------- USER / HW CONFIG -------------------------------------
// ============================================================================

// Set whether positive Z means BLUE (true) or RED (false)
static const bool POSITIVE_Z_IS_BLUE = true;
static const int SDA_PIN = 8;
static const int SCL_PIN = 9;

static const char ADV_PRESENCE_VALUE_RED  = 'R';
static const char ADV_PRESENCE_VALUE_BLU  = 'B';
static const char ADV_PRESENCE_VALUE_BOTH = 'T';

// this should never get sent now because to save power we do not transmit
// when nobody is on
static const char ADV_PRESENCE_VALUE_NONE = 'N';

// NOTE: on XIAO ESP32-C3 SDA/SCL are 4/5.
// If you actually use I2C on 4/5, move the LED to another pin.
#define LED_PIN        3
#define NUM_LEDS       1

// NEW: switch inputs (normally open to GND)
static const int BLUE_SWITCH_PIN = 1;  // closed => blue present
static const int RED_SWITCH_PIN  = 2;  // closed => red present

// NEW: LED wrapper instance (no global strip here)
StatusLED statusLed(LED_PIN, NUM_LEDS);

// NEW: debouncers for the switches
Bounce blueSwitchDebouncer;
Bounce redSwitchDebouncer;

// ============================================================================
// ------------------ QMC5883 baseline / EMA params ---------------------------
const float BASELINE_ALPHA = 0.05f;
const float OUTPUT_ALPHA   = 0.10f;
const unsigned long BASELINE_MS = 10000UL;

float baseX = 0, baseY = 0, baseZ = 0;
bool  baselineInit  = false;
bool  baselineDone  = false;
unsigned long tStart = 0;

bool  outputEmaInit = false;
float fDx = 0, fDy = 0, fDz = 0, fDt = 0;

// presence thresholds
const float MIN_ON_DT   = 300.0f;
const float STRONG_DT   = 1100.0f;  // NOTE: no longer used for LED, but kept for logging/strength
const float OFF_HYST_DT = 300.0f;

BleReporter reporter;

bool  g_playerPresent = false;  // magnetic presence only
float g_lastDt = 0.0f;

// NEW: flag telling us if the QMC is present & initialized
bool g_qmc_present = false;

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
  delay(200);
  Serial.println("Startup...");

  statusLed.begin(64);
  statusLed.off();

  // I2C init
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setTimeout(50);

  // NEW: switch pins with pullups
  pinMode(BLUE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RED_SWITCH_PIN,  INPUT_PULLUP);

  blueSwitchDebouncer.attach(BLUE_SWITCH_PIN);
  blueSwitchDebouncer.interval(20);  // 20 ms debounce

  redSwitchDebouncer.attach(RED_SWITCH_PIN);
  redSwitchDebouncer.interval(20);   // 20 ms debounce

  // CHANGED: capture whether the QMC is actually present
  g_qmc_present = initQMC5883P();   // auto-detect/init_qmc() from qmc5883.h
  if (!g_qmc_present) {
    Serial.println("QMC not detected; running in switch-only mode.");
    baselineDone = true;
  }

  tStart = millis();


  reporter.begin();
}

void loop() {
  // ----------------- Update debounced switch state -----------------
  blueSwitchDebouncer.update();
  redSwitchDebouncer.update();

  // Active LOW because of INPUT_PULLUP and normally-open to GND
  bool blueSwitchOn = (blueSwitchDebouncer.read() == LOW);
  bool redSwitchOn  = (redSwitchDebouncer.read() == LOW);

  // ----------------- Magnetic sensor path (optional) -----------------
  // Default: no magnetic team info
  char teamCharMag = 'R';

  if (g_qmc_present) {
    int16_t rx, ry, rz;
    if (!readQMC5883PData(rx, ry, rz)) {
      // If QMC was present at boot but a read fails, we keep old behavior:
      // bail out of this iteration to avoid using bogus data.
      statusLed.update();
      delay(10);
      return;
    }

    float x = rx;
    float y = ry;
    float z = rz;

    unsigned long now = millis();

    // ----------------- Baseline phase -----------------
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

      statusLed.setFastBlink(COLOR_YELLOW);
      statusLed.update();

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

    // ----------------- Run phase -----------------
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
    Serial.printf(">fDt: %0.1f\n", fDt);
    g_lastDt = fDt;

    // presence FSM (magnet-based only)
    if (!g_playerPresent) {
      if (fDt >= MIN_ON_DT) {
        g_playerPresent = true;
      }
    } else {
      if (fDt <= OFF_HYST_DT) {
        g_playerPresent = false;
      }
    }

    // Compute team from magnet Z sign when present.
    if (g_playerPresent) {
      teamCharMag = teamFromZ(fDz);
    }
  } else {
    // No QMC: ensure magnet presence does not contribute
    g_playerPresent = false;
  }

  // ----------------- Combine magnetic + switch presence -----------------
  bool bluePresent = false;
  bool redPresent  = false;

  // From magnet
  if (g_playerPresent) {
    if (teamCharMag == 'R') {
      redPresent = true;
    } else if (teamCharMag == 'B') {
      bluePresent = true;
    } else {
      // if sign is weird, don't set either
    }
  }

  // From switches
  if (blueSwitchOn) bluePresent = true;
  if (redSwitchOn)  redPresent  = true;

  bool anyPlayerPresent = (bluePresent || redPresent);


  char teamCharAdv = ADV_PRESENCE_VALUE_NONE;
  if (bluePresent && redPresent) {
    teamCharAdv = ADV_PRESENCE_VALUE_BOTH;  
  } else if (bluePresent) {
    teamCharAdv = ADV_PRESENCE_VALUE_BLU;
  } else if (redPresent) {
    teamCharAdv = ADV_PRESENCE_VALUE_RED;
  }

  // ----------------- LED state -----------------
  if (!anyPlayerPresent) {
    // Blinking green when ready to capture but nobody on
    statusLed.setFastBlink(COLOR_GREEN);
  } else {
    // Solid red when red only, solid blue when blue only,
    // solid white when both present.
    if (bluePresent && redPresent) {
      statusLed.setSolid(COLOR_PURPLE);  // both
    } else if (redPresent) {
      statusLed.setSolid(COLOR_RED);
    } else if (bluePresent) {
      statusLed.setSolid(COLOR_BLUE);
    } else {
      // Fallback
      statusLed.setSolid(COLOR_GRAY);
    }
  }

  // Debug for team presence
  if (anyPlayerPresent) {
    if (redPresent && !bluePresent) {
      Serial.println(">P: 1");  // red only
    } else if (bluePresent && !redPresent) {
      Serial.println(">P: 2");  // blue only
    } else if (bluePresent && redPresent) {
      Serial.println(">P: 3");  // both
    } else {
      Serial.println(">P: 4");  // should not happen
    }
  } else {
    Serial.println(">P: 0");
  }

  // BLE FSM (includes combined teamCharAdv)
  // If QMC is absent, fDt stays at its default (0.0f), which is fine.
  reporter.update(anyPlayerPresent, fDt, teamCharAdv);

  // Update LED blink timing
  statusLed.update();

  delay(10); // ~100 Hz
}
