#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Rgb.h"

// Single-LED indicator with foreground/background colors and duty-cycle blinking.
//
// - Background color = "owner"
// - Foreground color = "capturing"
// - Blink interval: full period in ms (e.g., 1000)
// - Blink rate: percentage of interval spent in foreground color (0..100)
//     0%   => always background
//     100% => always foreground
//     50%  => half the time fg, half bg
//
// You must call update(nowMs) frequently in loop(), passing your notion of time.

class FgBgLed {
public:
    FgBgLed(uint8_t pin, uint16_t count = 1)
        : _strip(count, pin, NEO_GRB + NEO_KHZ800),
          _fg(0, 255, 0),           // default fg = green
          _bg(0, 0, 0),             // default bg = black
          _blinkIntervalMs(1000),   // default 1s
          _blinkRatePercent(0),     // default 0% => solid bg
          _phaseStartMs(0),
          _initialized(false),
          _last(0, 0, 0) {}

    void begin(uint8_t brightness = 64) {
        _strip.begin();
        _strip.setBrightness(brightness);
        // Show initial background; timing is handled on first update()
        _renderColor(_bg, true);
    }

    // ----------------------------
    // Configuration
    // ----------------------------

    void setForeground(const Rgb& c) {
        _fg = c;
        // no immediate render; update() will handle it based on time/duty
    }

    void setBackground(const Rgb& c) {
        _bg = c;
        // no immediate render; update() will handle it based on time/duty
    }

    // Blink interval in ms (whole period)
    void setBlinkIntervalMs(uint32_t intervalMs) {
        if (intervalMs == 0) intervalMs = 1; // avoid divide-by-zero
        _blinkIntervalMs = intervalMs;
        // phase will be normalized on next update(nowMs)
    }

    // 0..100
    void setBlinkRatePercent(uint8_t percent) {
        if (percent > 100) percent = 100;
        _blinkRatePercent = percent;
        // phase handled on next update(nowMs)
    }

    uint8_t  getBlinkRatePercent() const { return _blinkRatePercent; }
    uint32_t getBlinkIntervalMs() const  { return _blinkIntervalMs; }

    // Convenience: force solid color
    void setSolid(const Rgb& c) {
        _fg = c;
        _bg = c;
        _blinkRatePercent = 100;
        _renderColor(_fg, true);
    }

    void off() {
        _fg = Rgb(0,0,0);
        _bg = Rgb(0,0,0);
        _blinkRatePercent = 0;
        _renderColor(_bg, true);
    }

    // ----------------------------
    // Main update – call in loop()
    // ----------------------------

    void update(uint32_t nowMs) {
        if (!_initialized) {
            _phaseStartMs = nowMs;
            _initialized  = true;
            _renderColor(_bg, true);
            return;
        }

        // Shortcut for trivial cases
        if (_blinkRatePercent == 0) {
            _renderColor(_bg);
            return;
        } else if (_blinkRatePercent == 100) {
            _renderColor(_fg);
            return;
        }

        uint32_t elapsed = nowMs - _phaseStartMs;
        if (elapsed >= _blinkIntervalMs) {
            if (_blinkIntervalMs == 0) {
                elapsed = 0;
            } else {
                elapsed = elapsed % _blinkIntervalMs;
            }
            _phaseStartMs = nowMs - elapsed;
        }

        uint32_t onTime = (static_cast<uint32_t>(_blinkIntervalMs) *
                           static_cast<uint32_t>(_blinkRatePercent)) / 100UL;

        bool on = (elapsed < onTime);
        if (on) {
            _renderColor(_fg);
        } else {
            _renderColor(_bg);
        }
    }

    // Handy for unit tests
    Rgb getLastRenderedColor() const {
        return _last;
    }

private:
    Adafruit_NeoPixel _strip;

    Rgb      _fg;
    Rgb      _bg;
    uint32_t _blinkIntervalMs;
    uint8_t  _blinkRatePercent;

    uint32_t _phaseStartMs;
    bool     _initialized;

    Rgb      _last;

    void _renderColor(const Rgb& c, bool force = false) {
        if (!force &&
            c.r == _last.r &&
            c.g == _last.g &&
            c.b == _last.b) {
            return; // nothing changed
        }

        _last = c;
        _strip.setPixelColor(0, _strip.Color(c.r, c.g, c.b));
        _strip.show();
    }
};
