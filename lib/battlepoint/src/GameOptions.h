#ifndef __INC_GAMEOPTIONS_H
#define __INC_GAMEOPTIONS_H
#include <Arduino.h>

typedef enum {
    AD,
    KOTH,
    CP
} GameMode;

typedef struct {
    GameMode _mode;
    uint8_t captureSeconds;
    uint8_t captureButtonThresholdSeconds;
    uint8_t startDelaySeconds;
    int timeLimitSeconds;
} GameOptions;

#endif