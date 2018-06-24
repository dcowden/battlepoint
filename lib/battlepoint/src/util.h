#ifndef __INC_UTIL_H
#define __INC_UTIL_H
#include <Arduino.h>
#include <FastLED.h>
int millis_to_seconds(long timeInMillis){
    return (int)timeInMillis / 1000;
}

int secondsSince(long startTimeMillis){
    long now = millis();
    return (int)(now - startTimeMillis) / 1000;
}

CRGB getFastLEDColor(TeamColor tc){
    switch(tc){
        case TeamColor::RED:
            return CRGB::Red;
        case TeamColor::BLUE:
            return CRGB::Blue;
        case TeamColor::BLACK:
            return CRGB::Black;
        case TeamColor::YELLOW:
            return CRGB::Yellow;
        case TeamColor::AQUA:
            return CRGB::Aqua;
        default:
            return CRGB::Black;
    }
}

#endif