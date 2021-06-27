#include <Arduino.h>
#include <FastLED.h>
#include <Teams.h>

#ifndef __INC_LEDMETER_H
#define __INC_LEDMETER_H

struct LedMeter {
    //zero based, and reversible
    uint8_t startIndex;
    uint8_t endIndex;
    int max_val;
    CRGB fgColor;
    CRGB bgColor;
};

CRGB getFastLEDColor(TeamColor tc);
int proportionalValue(int in_val, int in_max, int out_max );
void updateLedMeter(CRGB* leds, LedMeter meter, int val );
#endif