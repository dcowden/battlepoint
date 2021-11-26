#ifndef __INC_CLOCK_DISPLAY_H
#define __INC_CLOCK_DISPLAY_H
#include <Arduino.h>

typedef enum {
    YELLOW = 1,
    RED = 2,
    BLUE =3,
    GREEN=4,
    WHITE=5,
    BLACK=6
} ClockColor;

typedef struct {
    //stored values
    long start_time_millis=0;
    int game_duration_secs=0;
    int start_delay_secs=0;

    //computed values
    long lastupdate_time_millis=0;
    int game_elapsed_secs=0;
    int game_remaining_secs=0;
    int secs_till_start = 0;
    ClockColor color;
    boolean in_progress;
    boolean is_over;

} GameClockState;

void initGameClock(GameClockState* clockState,int start_delay_secs, int game_time_secs,long current_time_millis);
void updateGameClock(GameClockState* clockState, long current_time_millis);
void updateServoTime(int value_seconds, ClockColor color);
#endif