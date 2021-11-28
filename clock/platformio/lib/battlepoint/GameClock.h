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

typedef enum {
    NOT_STARTED = 1,
    COUNTING_TO_START = 2,
    IN_PROGRESS = 3,
    OVER = 4
} ClockState;

typedef struct {
    //stored values
    long start_time_millis=0;
    int game_duration_secs=0;
    int start_delay_secs=0;

    //computed values
    long lastupdate_time_millis=0;
    int game_elapsed_secs=0;
    int game_remaining_secs=0;
    int time_to_display_secs=0;
    int secs_till_start = 0;
    ClockState clockState;
} GameClockState;

ClockColor game_clock_color_for_state(GameClockState* clockState);

const char* get_state_desc(ClockState state);
void game_clock_configure(GameClockState* clockState,int start_delay_secs, int game_time_secs);
void game_clock_start(GameClockState* clockState,long current_time_millis);
void game_clock_update(GameClockState* clockState, long current_time_millis);

#endif