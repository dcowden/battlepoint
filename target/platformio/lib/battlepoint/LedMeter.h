#ifndef __INC_LEDMETER_H
#define __INC_LEDMETER_H
#include <Arduino.h>
#include <FastLED.h>
#include <Teams.h>

#define DEFAULT_MAX_VAL 10

typedef enum {
    FLASH_SLOW = 2000,
    FLASH_FAST = 200,
    FLASH_NONE = 0
} FlashInterval;

typedef struct  {
    //zero based, and reversible
    uint8_t startIndex;
    uint8_t endIndex;
    int max_val;
    int val;
    int flash_interval_millis; 
    int last_flash_millis;
    CRGB fgColor;
    CRGB bgColor;
} LedMeter;

CRGB getFastLEDColor(TeamColor tc);
int proportionalValue(int in_val, int in_max, int out_max );
void initMeter ( LedMeter* meter, int startIndex, int endIndex );
void configureMeter( LedMeter* meter, int max_val, int val, CRGB fg, CRGB bg);
void updateLedMeter(CRGB* leds, LedMeter meter );
#endif