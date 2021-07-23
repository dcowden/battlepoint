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
    CRGB fgColor;
    CRGB bgColor;
} LedMeter;


typedef struct {
    long last_flash_millis = 0 ;
    int flash_state = 0;
} LedFlashState;


typedef struct {
    LedMeter meter;
    LedFlashState flashState;
} LedController;

CRGB getFastLEDColor(TeamColor tc);
int proportionalValue(int in_val, int in_max, int out_max );
void initMeter ( LedMeter* meter, int startIndex, int endIndex );
void configureMeter( LedMeter* meter, int max_val, int val, CRGB fg, CRGB bg);
void updateLedMeter(CRGB* leds, LedMeter meter );
void updateController(CRGB* leds, LedController controller, long current_time_millis);
#endif