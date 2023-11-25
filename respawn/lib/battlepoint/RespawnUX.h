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
    RespawnTimer* timer;
} RespawnUX;

typedef struct{
    RespawnUX* player1;
    RespawnUX* player2;
    RespawnUX* player3;
    RespawnUX* player4;
} RespawnContext;


void contextDisableTimers(RespawnContext* context);
void contextUpdate(RespawnContext* context,long currentTimeMillis);
void initContext ( RespawnContext* context,CRGB* leds,int soundPin);
void signalNoSlotsAvailable(int soundPin);
void initRespawnUX(RespawnUX* respawnUX, CRGB* leds, uint8_t index, int soundPin, RespawnTimer* timer);
void updateRespawnUX(RespawnUX* respawnUX, long currentTimeMillis);
CRGB getColorForTimerState(RespawnTimerState state);
#endif