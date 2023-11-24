#ifndef __INC_RESPAWNUX_H
#define __INC_RESPAWNUX_H
#include <FastLED.h>
#include <RespawnTimer.h>



typedef enum {
    FLASH_SLOW = 800,
    FLASH_FAST = 400,
    FLASH_NONE = 0
} FlashInterval;

typedef struct{
    CRGB* leds;
    uint8_t index;
    int soundPin;
    bool notifiedStart;
    bool notifiedFinish;
} RespawnUX;

void initRespawnUX(RespawnUX* respawnUX, CRGB* leds, uint8_t index, int soundPin);
void updateRespawnUX(RespawnUX* respawnUX, RespawnTimerState state, long currentTimeMillis);
CRGB getColorForTimerState(RespawnTimerState state);
#endif