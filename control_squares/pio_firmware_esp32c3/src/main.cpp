#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string>

#include "esp_sleep.h"   // for light sleep

// ------------------ NeoPixel -------------
#include <Adafruit_NeoPixel.h>

// ------------------ QMC auto-detect ------
#include "qmc5883.h"

// ------------------ Teams / MiniCP / Colors / LED -------------
#include "Teams.h"
#include "minicp.h"
#include "ColorMapper.h"
#include "Rgb.h"
#include "FgBgLed.h"

// ------------------ BLE reporter ---------
#include "ble.h"

// ------------------ Debounce -------------
#include <Bounce2.h>

// ============================================================================
// --------------------- USER / HW CONFIG -------------------------------------
// ============================================================================

// We keep this flag and the color constants around for future use,
// but we no longer use field direction for team detection.
static const bool POSITIVE_Z_IS_BLUE = true;
static const int SDA_PIN = 8;
static const int SCL_PIN = 9;

static const char* TILE_ID = "CS-06";

static const char ADV_PRESENCE_VALUE_RED   = 'R';
static const char ADV_PRESENCE_VALUE_BLU   = 'B';
static const char ADV_PRESENCE_VALUE_BOTH  = 'T';
static const char ADV_PRESENCE_VALUE_NONE  = 'N';
// NEW: magnet-only presence, team indeterminate
static const char ADV_PRESENCE_VALUE_MAG   = 'M';

// NOTE: on XIAO ESP32-C3 SDA/SCL are 4/5.
// If you actually use I2C on 4/5, move the LED to another pin.
#define LED_PIN        3
#define NUM_LEDS       1

// switch inputs (normally open to GND)
static const int BLUE_SWITCH_PIN = 1;  // closed => blue present
static const int RED_SWITCH_PIN  = 2;  // closed => red present

// Capture timing for mini control point (10 seconds)
static const uint32_t CAPTURE_TIME_MS = 10000UL;

// Light sleep interval
static const uint64_t SLEEP_US = 50ULL * 1000ULL;  // 50 ms

// ============================================================================
// ------------------ QMC5883 baseline / EMA params ---------------------------

const float BASELINE_ALPHA   = 0.05f;
const float OUTPUT_ALPHA     = 0.50f;
const unsigned long BASELINE_MS = 10000UL;

float baseX = 0, baseY = 0, baseZ = 0;
bool  baselineInit  = false;
bool  baselineDone  = false;
unsigned long tStart = 0;

bool  outputEmaInit = false;
float fDx = 0, fDy = 0, fDz = 0, fDt = 0;

// presence thresholds (field magnitude vs baseline)
const float MIN_ON_DT   = 380.0f;
const float OFF_HYST_DT = 360.0f;

BleReporter reporter(TILE_ID);


// magnetic presence only (no team inference from field)
bool  g_playerPresent = false;
float g_lastDt        = 0.0f;

// flag telling us if the QMC is present & initialized
bool g_qmc_present = false;

// ============================================================================
// ------------------ Game / LED Objects --------------------------------------

// Mini control point: owner/capturing/progress logic
MiniControlPoint g_controlPoint(CAPTURE_TIME_MS);

// Single LED indicator with foreground/background + blink duty
FgBgLed g_statusLed(LED_PIN, NUM_LEDS);

// debouncers for the switches
Bounce blueSwitchDebouncer;
Bounce redSwitchDebouncer;

// ============================================================================
// ------------------- Arduino setup / loop -----------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("Startup...");

  g_statusLed.begin(64);
  g_statusLed.off();

  // I2C init
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setTimeout(50);

  // switch pins with pullups
  pinMode(BLUE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RED_SWITCH_PIN,  INPUT_PULLUP);

  blueSwitchDebouncer.attach(BLUE_SWITCH_PIN);
  blueSwitchDebouncer.interval(20);  // 20 ms debounce

  redSwitchDebouncer.attach(RED_SWITCH_PIN);
  redSwitchDebouncer.interval(20);   // 20 ms debounce

  // capture whether the QMC is actually present
  g_qmc_present = initQMC5883P();   // auto-detect/init_qmc() from qmc5883.h

  if (!g_qmc_present) {
    Serial.println("QMC not detected; running in switch-only mode.");
    baselineDone = true;  // skip baseline if no sensor
  }

  tStart = millis();

  g_controlPoint.reset();
  reporter.begin();
}

void loop() {
  uint32_t now = millis();

  // ----------------- Update debounced switch state -----------------
  blueSwitchDebouncer.update();
  redSwitchDebouncer.update();

  // Active LOW because of INPUT_PULLUP and normally-open to GND
  bool blueSwitchOn = (blueSwitchDebouncer.read() == LOW);
  bool redSwitchOn  = (redSwitchDebouncer.read() == LOW);

  // ----------------- Magnetic sensor path (presence only) -----------
  if (g_qmc_present) {
    int16_t rx, ry, rz;
    if (!readQMC5883PData(rx, ry, rz)) {
      // If QMC was present at boot but a read fails, bail out of this iteration
      // to avoid using bogus data.
      g_statusLed.update(now);
      delay(10);
      return;
    }

    float x = rx;
    float y = ry;
    float z = rz;

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

      // Baseline indicator: blinking yellow over black
      g_statusLed.setForeground(Rgb(255, 255, 0));   // yellow
      g_statusLed.setBackground(Rgb(0, 0, 0));       // black
      g_statusLed.setBlinkIntervalMs(500);           // 0.5s
      g_statusLed.setBlinkRatePercent(50);           // 50% duty
      g_statusLed.update(now);

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
    Serial.printf(">fDx: %0.1f\n", fDx);
    Serial.printf(">fDy: %0.1f\n", fDy);
    Serial.printf(">fDz: %0.1f\n", fDz);
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

  } else {
    // No QMC: ensure magnetic presence does not contribute
    g_playerPresent = false;
  }

  // ----------------- Combine magnetic + switch presence -----------------
  bool magPresent  = g_playerPresent;   // man-on from magnet only
  bool bluePresent = false;
  bool redPresent  = false;

  // From switches (definitive team)
  if (blueSwitchOn) bluePresent = true;
  if (redSwitchOn)  redPresent  = true;

  bool anyPlayerPresent = (magPresent || bluePresent || redPresent);

  char teamCharAdv = ADV_PRESENCE_VALUE_NONE;

  if (bluePresent && redPresent) {
    // both switches: keep previous behavior
    teamCharAdv = ADV_PRESENCE_VALUE_BOTH;
  } else if (bluePresent) {
    teamCharAdv = ADV_PRESENCE_VALUE_BLU;
  } else if (redPresent) {
    teamCharAdv = ADV_PRESENCE_VALUE_RED;
  } else if (magPresent) {
    // magnet-only presence: team unknown
    teamCharAdv = ADV_PRESENCE_VALUE_MAG;
  } else {
    teamCharAdv = ADV_PRESENCE_VALUE_NONE;
  }

  // ----------------- Mini control point update -----------------

  // Interpret presence as:
  //   redPresent  -> Team::RED
  //   bluePresent -> Team::BLU
  //   magPresent  -> Team::NEUTRAL
  g_controlPoint.update(now, redPresent, bluePresent, magPresent);

  Team owner     = g_controlPoint.owner();
  Team capturing = g_controlPoint.capturing();
  uint8_t progress = g_controlPoint.progressPercent();  // 0..100

  // ----------------- LED state via ColorMapper + FgBgLed ---------

  // Background = owner, foreground = capturing / opposite as per ColorMapper
  Rgb bg = ColorMapper::backgroundFor(owner);
  Rgb fg = ColorMapper::foregroundFor(owner, capturing);


g_controlPoint.applyLedPattern(g_statusLed,
  now,
  ColorMapper::fromTeamColor(TeamColor::COLOR_RED),
  ColorMapper::fromTeamColor(TeamColor::COLOR_BLUE),
  ColorMapper::fromTeamColor(TeamColor::COLOR_GREEN),
  ColorMapper::fromTeamColor(TeamColor::COLOR_BLACK)
);
g_statusLed.update(now);

  // ----------------- Debug for presence breakdown -----------------
  if (!anyPlayerPresent) {
    Serial.println(">P: 0");  // nobody
  } else if (bluePresent && !redPresent && !magPresent) {
    Serial.println(">P: 1");  // blue only (switch)
  } else if (redPresent && !bluePresent && !magPresent) {
    Serial.println(">P: 2");  // red only (switch)
  } else if (bluePresent && redPresent) {
    Serial.println(">P: 3");  // both switches
  } else if (magPresent && !bluePresent && !redPresent) {
    Serial.println(">P: 4");  // magnet only
  } else {
    Serial.println(">P: 5");  // mixed case (switch + magnet)
  }

  // ----------------- BLE FSM (includes combined teamCharAdv) ------
  reporter.update(anyPlayerPresent, fDt, teamCharAdv);

  // ----------------- Power management -----------------
  if (!anyPlayerPresent) {
    //Serial.println("Sleep");
    //Serial.flush();

    // Use the pattern that worked in your original sketch: enable timer every time
    const uint64_t sleep_us = 50ULL * 1000ULL;  // 50 ms in microseconds
    esp_sleep_enable_timer_wakeup(sleep_us);
    esp_light_sleep_start();
    // Then loop() runs again
  } else {
    // Player present: keep responsiveness high
    delay(10); // ~100 Hz
  }
}
