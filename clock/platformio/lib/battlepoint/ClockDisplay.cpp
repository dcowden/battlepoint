#include <Arduino.h>
#include <ClockDisplay.h>

#define SECONDS_PER_MINUTE 60

void initGameClock(GameClockState* clockState,int start_delay_secs, int game_time_secs,long current_time_millis){
    clockState->start_time_millis = current_time_millis;
    clockState->game_duration_secs = game_time_secs;
    clockState->start_delay_secs = game_time_secs;
    clockState->color = ClockColor::BLACK;
    clockState->game_elapsed_secs=0;
    clockState->secs_till_start=start_delay_secs;
    clockState->game_remaining_secs=game_time_secs;
    clockState->in_progress = false;
    clockState->is_over = false;
}

void updateGameClock(GameClockState* clockState, long current_time_millis){
    clockState->lastupdate_time_millis = current_time_millis;
    long elapsed_secs = current_time_millis- clockState->start_time_millis;
    long game_elapsed_secs = elapsed_secs - clockState->start_delay_secs;
    
    if ( game_elapsed_secs < 0){
        clockState->secs_till_start = -game_elapsed_secs;
        clockState->in_progress = false;
        clockState->color = ClockColor::WHITE;
    }
    else if ( game_elapsed_secs >= clockState->game_duration_secs){
        clockState->secs_till_start = 0;
        clockState->in_progress = false;
        clockState->game_elapsed_secs = clockState->game_duration_secs;
        clockState->game_remaining_secs = 0;        
        clockState->color = ClockColor::RED;
    }
    else{
        clockState->secs_till_start = 0;
        clockState->in_progress = true;
        clockState->game_elapsed_secs = game_elapsed_secs;
        int game_remaining_secs = clockState->game_duration_secs - game_elapsed_secs;
        clockState->game_remaining_secs = game_remaining_secs;
        if (game_remaining_secs <=10 ){
            clockState->color = ClockColor::RED;
        }
        else{
            clockState->color = ClockColor::YELLOW;
        }
    }

} 

void updateServoValue(int value, ClockColor color){
    //do the stuff
}

void updateServoTime(int value_seconds, ClockColor color){
    int value_to_show = value_seconds;
    if ( value_seconds > SECONDS_PER_MINUTE){
        value_to_show = value_seconds / SECONDS_PER_MINUTE;
    }
    updateServoValue(value_to_show,color);
}