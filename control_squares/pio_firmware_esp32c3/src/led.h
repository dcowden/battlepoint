#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class StatusLED {
public:
  // Fast blink definition: ON 250 ms, OFF 750 ms
  static constexpr uint32_t FAST_ON_MS  = 80;
  static constexpr uint32_t FAST_OFF_MS = 750;

  StatusLED(uint8_t pin, uint16_t count)
    : _strip(count, pin, NEO_GRB + NEO_KHZ800),
      _r(0), _g(0), _b(0),
      _haveColor(false),
      _mode(Mode::OFF),
      _blinkOn(false),
      _lastToggle(0) {}

  void begin(uint8_t brightness = 64) {
    _strip.begin();
    _strip.setBrightness(brightness);
    off();
  }

  // Solid color API
  void setSolid(uint8_t r, uint8_t g, uint8_t b) {
    // If mode and color are unchanged, do nothing
    if (_mode == Mode::SOLID && _haveColor &&
        r == _r && g == _g && b == _b) {
      return;
    }

    _mode      = Mode::SOLID;
    _r         = r;
    _g         = g;
    _b         = b;
    _haveColor = true;
    _blinkOn   = true;     // treat solid as "always on"
    applyColor(true);

    Serial.printf("LED SOLID -> %s (%u,%u,%u)\n",
                  colorName(r, g, b), r, g, b);
  }

  // Fast blink (250 ms on, 750 ms off)
  void setFastBlink(uint8_t r, uint8_t g, uint8_t b) {
    // If mode and color are unchanged, do nothing
    if (_mode == Mode::FAST_BLINK && _haveColor &&
        r == _r && g == _g && b == _b) {
      return;
    }

    _mode      = Mode::FAST_BLINK;
    _r         = r;
    _g         = g;
    _b         = b;
    _haveColor = true;
    _blinkOn   = true;
    _lastToggle = millis();
    applyColor(true);

    Serial.printf("LED FAST BLINK -> %s (%u,%u,%u)\n",
                  colorName(r, g, b), r, g, b);
  }

  void off() {
    if (_mode == Mode::OFF && !_blinkOn && !_haveColor) {
      return;
    }
    _mode      = Mode::OFF;
    _blinkOn   = false;
    _haveColor = false;
    _r = _g = _b = 0;
    applyColor(false);
    Serial.println("LED -> Off");
  }

  // Call this once per loop to handle blinking
  void update() {
    if (_mode != Mode::FAST_BLINK) {
      return; // nothing to time
    }

    uint32_t now = millis();
    uint32_t interval = _blinkOn ? FAST_ON_MS : FAST_OFF_MS;

    if (now - _lastToggle >= interval) {
      _blinkOn = !_blinkOn;
      _lastToggle = now;
      applyColor(_blinkOn);
    }
  }

  // Convenience helpers if you want them
  void solidRed()    { setSolid(255, 0,   0); }
  void solidBlue()   { setSolid(0,   0, 255); }
  void solidGreen()  { setSolid(0, 180,  0); }
  void solidPurple() { setSolid(200, 0, 200); }

  void blinkPurpleFast() { setFastBlink(200, 0, 200); }
  void blinkGreenFast()  { setFastBlink(0, 180, 0); }

private:
  enum class Mode : uint8_t {
    OFF,
    SOLID,
    FAST_BLINK
  };

  Adafruit_NeoPixel _strip;
  uint8_t  _r, _g, _b;
  bool     _haveColor;
  Mode     _mode;
  bool     _blinkOn;
  uint32_t _lastToggle;

  void applyColor(bool on) {
    uint8_t r = on ? _r : 0;
    uint8_t g = on ? _g : 0;
    uint8_t b = on ? _b : 0;

    _strip.setPixelColor(0, _strip.Color(r, g, b));
    _strip.show();
  }

  static const char* colorName(uint8_t r, uint8_t g, uint8_t b) {
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
