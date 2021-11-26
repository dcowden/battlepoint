#ifndef __INC_CLOCK_DISPLAY_H
#define __INC_CLOCK_DISPLAY_H
#include <Arduino.h>

typedef enum {
    YELLOW = 1,
    RED = 2,
    BLUE =3,
    GREEN=4,
    WHITE=5
} ClockColor;

typedef struct {
    long start_time_millis=0;
    long current_time_millis=0;
    int game_elapsed_secs=0;
    int game_remaining_secs=0;
    int game_duration_secs=0;
    int start_delay_secs=0;
} ClockState;

void updateClockDisplay(ClockState* clockState, long current_time_millis);

void setClockDisplay(int seconds_remaining, ClockColor cc);

#endif