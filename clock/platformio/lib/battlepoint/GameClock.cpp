#include <Arduino.h>
#include <GameClock.h>

#define SECONDS_PER_MINUTE 60

const char* get_state_desc(ClockState state){
    if ( state == COUNTING_TO_START ){
        return "Counting..";
    }
    else if ( state ==  IN_PROGRESS){
        return "running";
    }
    else if ( state == OVER){
        return "Over";
    }
    else{
        return "?";
    }
}

ClockColor game_clock_color_for_state(GameClockState* clockState){
    ClockState cs = clockState->clockState;
    if ( ClockState::NOT_STARTED  == cs || ClockState::COUNTING_TO_START == cs){
        return ClockColor::YELLOW;
    }
    else if ( ClockState::IN_PROGRESS  == cs){
        if ( clockState->game_remaining_secs < 10){
            return ClockColor::RED;
        }
        else{
            return ClockColor::BLUE;
        }
    }
    else if ( cs == ClockState::OVER ) {
        return ClockColor::RED;
    }
}
void game_clock_configure(GameClockState* clockState,int start_delay_secs, int game_time_secs){
    clockState->game_duration_secs = game_time_secs;
    clockState->start_delay_secs = start_delay_secs;

    clockState->game_elapsed_secs=0;
    clockState->secs_till_start=start_delay_secs;
    clockState->game_remaining_secs=game_time_secs;
    clockState->clockState = ClockState::NOT_STARTED;
    clockState->time_to_display_secs = 0;
}
void game_clock_start(GameClockState* clockState,long current_time_millis){
    clockState->clockState = ClockState::COUNTING_TO_START;
    clockState->lastupdate_time_millis = current_time_millis;
    clockState->start_time_millis = current_time_millis;
}

void game_clock_update(GameClockState* clockState, long current_time_millis){
    clockState->lastupdate_time_millis = current_time_millis;
    if ( clockState->clockState == ClockState::NOT_STARTED){
        return;
    }

    long secs_elapsed_since_start = (current_time_millis- clockState->start_time_millis)/1000;

    if ( secs_elapsed_since_start < clockState->start_delay_secs ){
        clockState->clockState = ClockState::COUNTING_TO_START;
        int seconds_left_till_start = clockState->start_delay_secs - secs_elapsed_since_start;
        clockState->secs_till_start = seconds_left_till_start;
        clockState->time_to_display_secs = seconds_left_till_start;
        clockState->game_elapsed_secs=0;
    }
    else{
        int seconds_since_game_start = secs_elapsed_since_start - clockState->start_delay_secs;
        if ( seconds_since_game_start <= clockState->game_duration_secs  ){
            //game running
            int seconds_left_in_game = clockState->game_duration_secs - seconds_since_game_start;
            clockState->clockState = ClockState::IN_PROGRESS;
            clockState->secs_till_start = 0;
            clockState->time_to_display_secs = seconds_left_in_game;
            clockState->game_elapsed_secs=seconds_since_game_start; 
            clockState->game_remaining_secs=clockState->game_duration_secs -  seconds_since_game_start;          
        }
        else{
            //game over
            clockState->clockState = ClockState::OVER;
            clockState->secs_till_start = 0;
            clockState->time_to_display_secs = 0;
            clockState->game_elapsed_secs=clockState->game_duration_secs;  
            clockState->game_remaining_secs=0;              
        }
    }

}