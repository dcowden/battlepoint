#include <Wire.h>
#include <Adafruit_QMC5883P.h>

Adafruit_QMC5883P qmc;

// ---- config ----
const float ALPHA = 0.05f;                 // EMA alpha for baseline
const unsigned long BASELINE_MS = 5000UL;  // build baseline for 5s
const unsigned long SAMPLE_MS   = 10UL;    // 100 Hz

// ---- baseline state ----
float baseX = 0.0f;
float baseY = 0.0f;
float baseZ = 0.0f;
bool baselineInit  = false;
bool baselineDone  = false;

unsigned long tStart    = 0;
unsigned long tLastSample = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for USB */ }

  Wire.begin();

  if (!qmc.begin()) {
    Serial.println("QMC5883P not found. Check wiring.");
    while (1) delay(500);
  }

  // force it into the mode that we KNOW works on your board
  qmc.setMode(QMC5883P_MODE_CONTINUOUS);
  qmc.setODR(QMC5883P_ODR_100HZ);
  qmc.setOSR(QMC5883P_OSR_4);
  qmc.setDSR(QMC5883P_DSR_1);
  qmc.setRange(QMC5883P_RANGE_8G);

  tStart = millis();
  tLastSample = millis();

  Serial.println("QMC5883P ready, building EMA baseline for 5s...");
}

void loop() {
  unsigned long now = millis();
  if (now - tLastSample < SAMPLE_MS) {
    return; // keep 100 Hz
  }
  tLastSample = now;

  int16_t rx, ry, rz;
  // IMPORTANT: don't wait for dataReady; just read like the working sample
  if (!qmc.getRawMagnetic(&rx, &ry, &rz)) {
    // don't make Teleplot try to plot this
    Serial.println("read failed");
    return;
  }

  // use floats for math
  float x = rx;
  float y = ry;
  float z = rz;

  // ================= BASELINE PHASE =================
  if (!baselineDone) {
    if (!baselineInit) {
      baseX = x;
      baseY = y;
      baseZ = z;
      baselineInit = true;
    } else {
      baseX = ALPHA * x + (1.0f - ALPHA) * baseX;
      baseY = ALPHA * y + (1.0f - ALPHA) * baseY;
      baseZ = ALPHA * z + (1.0f - ALPHA) * baseZ;
    }

    // tell teleplot we're still baselining
    Serial.println(F(">baseline_building:1"));

    if (now - tStart >= BASELINE_MS) {
      baselineDone = true;
      // print once WITHOUT '>' so Teleplot ignores it
      Serial.println(F("=== Baseline established ==="));
      Serial.print(F("base_x: ")); Serial.println(baseX, 3);
      Serial.print(F("base_y: ")); Serial.println(baseY, 3);
      Serial.print(F("base_z: ")); Serial.println(baseZ, 3);
      Serial.println(F("==========================="));
    }
    return;
  }

  // ================= RUN PHASE =================
  float dx = x - baseX;
  float dy = y - baseY;
  float dz = z - baseZ;
  float diffMag = sqrtf(dx * dx + dy * dy + dz * dz);

  // Teleplot lines
  Serial.print(F(">mag_dx:")); Serial.println(dx, 3);
  Serial.print(F(">mag_dy:")); Serial.println(dy, 3);
  Serial.print(F(">mag_dz:")); Serial.println(dz, 3);
  Serial.print(F(">mag_diff_strength:")); Serial.println(diffMag, 3);

  // raw too
  Serial.print(F(">mag_raw_x:")); Serial.println(x, 3);
  Serial.print(F(">mag_raw_y:")); Serial.println(y, 3);
  Serial.print(F(">mag_raw_z:")); Serial.println(z, 3);

  // flag so you can hide the baseline curve in Teleplot
  Serial.println(F(">baseline_building:0"));
}
