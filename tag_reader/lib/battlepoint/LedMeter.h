#ifndef __INC_LEDMETER_H
#define __INC_LEDMETER_H
#include <Arduino.h>
#include <FastLED.h>
#include <Teams.h>
#include<ArduinoLog.h>

#define DEFAULT_MAX_VAL 10

typedef enum {
    FLASH_SLOW = 800,
    FLASH_FAST = 400,
    FLASH_NONE = 0
} FlashInterval;

typedef struct  {
    //zero based, and reversible
    uint8_t startIndex;
    uint8_t endIndex;
    CRGB* leds;
    int max_val;
    int val;
    int flash_interval_millis; 
    CRGB fgColor;
    CRGB bgColor;
    const char* name;
    int flash_state;
    long last_flash_millis;
} LedMeter;


CRGB getFastLEDColor(TeamColor tc);
int proportionalValue(int in_val, int in_max, int out_max );
void initMeter ( LedMeter* meter, const char* name, CRGB* leds, int startIndex, int endIndex );

//TODO: same as setMeterValues?
void configureMeter( LedMeter* meter, int max_val, int val, CRGB fg, CRGB bg);

//TODO: this version should be eliminated
void updateLedMeter(LedMeter* meter );
void updateLedMeter(LedMeter* meter, long current_time_millis );
void setMeterValues (LedMeter* meter, int val, int max_val, CRGB fgColor, CRGB bgColor );
void setMeterValue(LedMeter* meter, int val, CRGB fgColor );
void setMeterValue(LedMeter* meter, int val, CRGB fgColor, long flash_interval_millis);
#endif