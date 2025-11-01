#include <Wire.h>

// ---------------- CONFIG ----------------
const int QMC5883P_ADDR = 0x2C;

// EMA for baseline build
const float BASELINE_ALPHA = 0.05f;
// EMA for plotted output (Dx, Dy, Dz, Dt)
const float OUTPUT_ALPHA   = 0.10f;

// how long to build baseline
const unsigned long BASELINE_MS = 5000UL;

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

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  initQMC5883P();
  tStart = millis();
  Serial.println("Building 5 s EMA baseline...");
}

// ------------------------------------------------------------
void loop() {
  int16_t rx, ry, rz;
  if (!readQMC5883PData(rx, ry, rz)) {
    // don't spam teleplot on failures
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
  // raw diffs
  float dx = x - baseX;
  float dy = y - baseY;
  float dz = z - baseZ;
  float dt = magnitude3D(dx, dy, dz);
  float rawMag = magnitude3D(x, y, z);

  // EMA on output diffs
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

  // Teleplot = smoothed values
  Serial.print(F(">DX:")); Serial.println(fDx, 2);
  Serial.print(F(">DY:")); Serial.println(fDy, 2);
  Serial.print(F(">DZ:")); Serial.println(fDz, 2);
  Serial.print(F(">DT:")); Serial.println(fDt, 3);

  // optional: raw magnitude for debugging
  Serial.print(F(">RAW_MAG:")); Serial.println(rawMag, 3);

  // tells the plot we're past baselining
  Serial.println(F(">baseline_building:0"));

  delay(10);   // ~100 Hz
}
