#pragma once

#include <Arduino.h>
#include "Teams.h"
#include "Rgb.h"      // for Rgb used in LED helper

#include "FgBgLed.h"

// MiniControlPoint
//
// Logical control point with time-based capture.
// Uses your Team enum:
//
//   RED, BLU, NEUTRAL, NOBODY, ALL
//
// Semantics (for this class):
//   - RED / BLU: normal teams
//   - NEUTRAL: unknown/magnet third-side capture
//   - NOBODY: no owner / no capturer
//   - ALL: not used by this class; reserved for your other code
//
// Behavior:
//   - _owner     : which Team currently owns the point
//   - _capturing : which Team is currently accumulating capture progress
//   - captureTimeMs: millis required to capture from 0% to 100%
//
//   - If exactly one side is present (red switch, blue switch, or magnet):
//        * if owner is NOBODY or other side → start capturing as that side
//        * progress increases while same side remains
//        * if a different side stands while mid-capture → progress decays
//   - If contested (2+ sources) → freeze progress
//   - If nobody on → progress decays
//   - When progress hits 100% → owner = capturing side, capturing resets
//
// No LED or color knowledge in the core logic; LED helper just *reads* state.

class MiniControlPoint {
public:
    enum class Presence : uint8_t {
        NONE,
        RED,
        BLU,
        NEUTRAL,    // magnet-only presence
        CONTESTED
    };

    explicit MiniControlPoint(uint32_t captureTimeMs = 10000UL)
        : _captureTimeMs(captureTimeMs ? captureTimeMs : 1),
          _owner(Team::NOBODY),
          _capturing(Team::NOBODY),
          _presence(Presence::NONE),
          _progressMs(0),
          _lastUpdateMs(0),
          _initialized(false),
          _lastOwnerForLed(Team::NOBODY),
          _celebrationActive(false),
          _celebrationStartMs(0) {}

    void reset() {
        _owner        = Team::NOBODY;
        _capturing    = Team::NOBODY;
        _presence     = Presence::NONE;
        _progressMs   = 0;
        _lastUpdateMs = 0;
        _initialized  = false;

        _lastOwnerForLed      = Team::NOBODY;
        _celebrationActive    = false;
        _celebrationStartMs   = 0;
    }

    void setCaptureTimeMs(uint32_t captureTimeMs) {
        _captureTimeMs = (captureTimeMs == 0) ? 1 : captureTimeMs;
    }

    uint32_t getCaptureTimeMs() const { return _captureTimeMs; }

    // Main update – call in loop()
    //
    // nowMs         : current time in ms (e.g., from millis())
    // redPresent    : true if red switch active
    // bluPresent    : true if blue switch active
    // neutralPresent: true if magnet present
    void update(uint32_t nowMs,
                bool redPresent,
                bool bluPresent,
                bool neutralPresent) {
        if (!_initialized) {
            _lastUpdateMs = nowMs;
            _initialized  = true;
            _presence     = _calcPresence(redPresent, bluPresent, neutralPresent);
            return;
        }

        uint32_t dt = nowMs - _lastUpdateMs;
        _lastUpdateMs = nowMs;

        _presence = _calcPresence(redPresent, bluPresent, neutralPresent);

        // Contest: freeze progress
        if (_presence == Presence::CONTESTED) {
            return;
        }

        // Determine which team (if any) is present
        Team presentTeam = _presenceToTeam(_presence);

        // Nobody on point: decay toward 0
        if (presentTeam == Team::NOBODY) {
            _decProgress(dt);
            return;
        }

        // Exactly one logical side on the point:
        // Start capturing if no current capturer and (owner is NOBODY or other side)
        if (_capturing == Team::NOBODY) {
            if (_owner == Team::NOBODY || presentTeam != _owner) {
                _capturing = presentTeam;
            }
        }

        if (_capturing == presentTeam) {
            // Actively capturing
            _incProgress(dt);
        } else {
            // Other side stands on point while we're mid-capture → decay
            _decProgress(dt);
        }

        _checkCapture();
    }

    // ------------------
    // State accessors
    // ------------------

    Team owner() const        { return _owner; }
    Team capturing() const    { return _capturing; }
    Presence presence() const { return _presence; }

    bool isContested() const { return _presence == Presence::CONTESTED; }

    // 0..100
    uint8_t progressPercent() const {
        if (_captureTimeMs == 0) return 0;
        uint32_t p = (_progressMs * 100UL) / _captureTimeMs;
        if (p > 100UL) p = 100UL;
        return static_cast<uint8_t>(p);
    }

    // ---------------------------------------------------------------------
    // LED helper: "pulse frequency = progress" using FgBgLed
    //
    // - Background color = current owner
    // - Foreground color = capturing side
    // - Blink interval shrinks as progress grows (1.0s → 0.2s)
    // - Duty cycle is small (~20%) so it looks like brief pulses
    // - On capture completion (owner changes from non-owner to a team):
    //      short celebration: ~800ms of 80ms on/off flashing
    //
    // Extra rule:
    //   - When nobody is physically on the point AND there is no owner
    //     (owner == NOBODY), emit a very brief GREEN blink once per second.
    //
    // Call this AFTER update(), once per loop, then call led.update(nowMs).
    // ---------------------------------------------------------------------
    void applyLedPattern(FgBgLed& led,
                         uint32_t nowMs,
                         const Rgb& redColor,
                         const Rgb& bluColor,
                         const Rgb& neutralColor,
                         const Rgb& nobodyColor) {
        // Detect owner changes for capture celebration
        if (_lastOwnerForLed != _owner) {
            // Only celebrate when we actually have a real owner
            if (_owner == Team::RED || _owner == Team::BLU || _owner == Team::NEUTRAL) {
                _celebrationActive  = true;
                _celebrationStartMs = nowMs;
            } else {
                _celebrationActive = false;
            }
            _lastOwnerForLed = _owner;
        }

        const uint32_t CELEBRATION_TOTAL_MS = 800UL;   // ~5 cycles of 80ms on/off

        Rgb ownerColor = _colorForTeam(_owner, redColor, bluColor, neutralColor, nobodyColor);

        // 1) Capture celebration: 80ms on / 80ms off rapid flash
        if (_celebrationActive) {
            uint32_t elapsed = nowMs - _celebrationStartMs;
            if (elapsed >= CELEBRATION_TOTAL_MS) {
                _celebrationActive = false;
            } else {
                // Use FgBgLed as a classic 50% duty blink during celebration.
                led.setBackground(ownerColor);
                led.setForeground(ownerColor);      // same color; visual comes from blinking
                led.setBlinkIntervalMs(160UL);      // 80ms on + 80ms off
                led.setBlinkRatePercent(50);        // 50% duty
                return;
            }
        }

        uint8_t progress = progressPercent();
        bool hasCaptureProgress = (_capturing != Team::NOBODY) && (progress > 0);

        // 2) Nobody on the point AND no current owner → brief green heartbeat every second.
        if (_presence == Presence::NONE && _owner == Team::NOBODY) {
            static const Rgb GREEN_HEARTBEAT(0, 180, 0);  // tweak as needed

            led.setBackground(nobodyColor);         // whatever you define for "unowned"
            led.setForeground(GREEN_HEARTBEAT);     // brief green pulse on top
            led.setBlinkIntervalMs(1000UL);         // 1 second period
            led.setBlinkRatePercent(10);            // ~100ms on, 900ms off
            return;
        }

        // 3) No capture in progress → solid owner color
        if (!hasCaptureProgress) {
            led.setSolid(ownerColor);
            return;
        }

        // 4) Capturing in progress → pulse frequency encodes progress
        Rgb capColor = _colorForTeam(_capturing, redColor, bluColor, neutralColor, nobodyColor);

        // Map 0..100% progress to 1000ms..200ms interval
        const uint32_t MAX_INTERVAL_MS = 1000UL;
        const uint32_t MIN_INTERVAL_MS = 200UL;

        uint32_t interval = MAX_INTERVAL_MS;
        if (progress >= 100) {
            interval = MIN_INTERVAL_MS;
        } else {
            uint32_t span = MAX_INTERVAL_MS - MIN_INTERVAL_MS;
            interval = MAX_INTERVAL_MS - (span * progress) / 100UL;
            if (interval < MIN_INTERVAL_MS) interval = MIN_INTERVAL_MS;
        }

        led.setBackground(ownerColor);
        led.setForeground(capColor);
        led.setBlinkIntervalMs(interval);
        led.setBlinkRatePercent(20);   // short pulse (~20% of the period)
    }

private:
    uint32_t _captureTimeMs;
    Team     _owner;
    Team     _capturing;
    Presence _presence;

    uint32_t _progressMs;
    uint32_t _lastUpdateMs;
    bool     _initialized;

    // LED-helper-only state (does not affect capture logic)
    Team     _lastOwnerForLed;
    bool     _celebrationActive;
    uint32_t _celebrationStartMs;

    static Presence _calcPresence(bool red, bool blu, bool neutral) {
        uint8_t count = (red ? 1 : 0) + (blu ? 1 : 0) + (neutral ? 1 : 0);
        if (count == 0) return Presence::NONE;
        if (count > 1)  return Presence::CONTESTED;
        if (red)        return Presence::RED;
        if (blu)        return Presence::BLU;
        if (neutral)    return Presence::NEUTRAL;
        return Presence::NONE; // shouldn't hit
    }

    // Map presence to which Team we treat as the active "capturing side":
    //
    // Presence::RED     -> Team::RED
    // Presence::BLU     -> Team::BLU
    // Presence::NEUTRAL -> Team::NEUTRAL (third/unknown party)
    // other             -> Team::NOBODY
    static Team _presenceToTeam(Presence p) {
        switch (p) {
            case Presence::RED:      return Team::RED;
            case Presence::BLU:      return Team::BLU;
            case Presence::NEUTRAL:  return Team::NEUTRAL;
            default:                 return Team::NOBODY;
        }
    }

    void _incProgress(uint32_t dt) {
        _progressMs += dt;
        if (_progressMs > _captureTimeMs) {
            _progressMs = _captureTimeMs;
        }
    }

    void _decProgress(uint32_t dt) {
        if (dt >= _progressMs) {
            _progressMs = 0;
            _capturing  = Team::NOBODY;
        } else {
            _progressMs -= dt;
        }
    }

    void _checkCapture() {
        if (_progressMs >= _captureTimeMs && _capturing != Team::NOBODY) {
            _owner      = _capturing;
            _capturing  = Team::NOBODY;
            _progressMs = 0;
        }
    }

    static Rgb _colorForTeam(Team t,
                             const Rgb& redColor,
                             const Rgb& bluColor,
                             const Rgb& neutralColor,
                             const Rgb& nobodyColor) {
        switch (t) {
            case Team::RED:     return redColor;
            case Team::BLU:     return bluColor;
            case Team::NEUTRAL: return neutralColor;
            case Team::NOBODY:
            default:            return nobodyColor;
        }
    }
};
