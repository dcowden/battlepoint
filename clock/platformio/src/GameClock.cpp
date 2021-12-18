#include <Arduino.h>
#include <GameClock.h>
#include <sound.h>
#include <FastLED.h>

#define SECONDS_PER_MINUTE 60
#define WARNING_TIME_LEFT_SECS 15
#define MSG_WAIT_DELAY_MS 400
#define MSG_VICTORY_DELAY_MS 5000

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
        if ( clockState->game_remaining_secs < WARNING_TIME_LEFT_SECS){
            return ClockColor::YELLOW;
        }
        else{
            return ClockColor::GREEN;
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
    sound_play(SND_SOUNDS_ANNOUNCER_TIME_ADDED);
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

        if ( seconds_left_till_start == 30 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_20SEC);
        }
        else if ( seconds_left_till_start == 20 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_30SEC);
        }
        else if ( seconds_left_till_start == 10 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_10SEC);
        }
        else if ( seconds_left_till_start == 5 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_5SEC);
            FastLED.delay(MSG_WAIT_DELAY_MS);
        }     
        else if ( seconds_left_till_start == 4 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_4SEC);
            FastLED.delay(MSG_WAIT_DELAY_MS);
        } 
        else if ( seconds_left_till_start == 3 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_3SEC);
            FastLED.delay(MSG_WAIT_DELAY_MS);
        }     
        else if ( seconds_left_till_start == 2 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_2SEC);
            FastLED.delay(MSG_WAIT_DELAY_MS);
        }                             
        else if ( seconds_left_till_start == 1 ){
            sound_play(SND_SOUNDS_ANNOUNCER_BEGINS_1SEC);
            FastLED.delay(MSG_WAIT_DELAY_MS);
        }                                     
    }
    else{
        int seconds_since_game_start = secs_elapsed_since_start - clockState->start_delay_secs;

        if ( seconds_since_game_start == 0 ){
            sound_play(SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4);
        }
        if ( seconds_since_game_start < clockState->game_duration_secs  ){
            //game running
            int seconds_left_in_game = clockState->game_duration_secs - seconds_since_game_start;
            clockState->clockState = ClockState::IN_PROGRESS;
            clockState->secs_till_start = 0;
            clockState->time_to_display_secs = seconds_left_in_game;
            clockState->game_elapsed_secs=seconds_since_game_start; 
            clockState->game_remaining_secs=clockState->game_duration_secs -  seconds_since_game_start;          

            //TODO: lookup table? 
            if ( seconds_left_in_game == 120 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_2MIN);
            }
            else if ( seconds_left_in_game == 60 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_60SEC);
            }            
            else if ( seconds_left_in_game == 30 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_30SEC);
            }
            else if ( seconds_left_in_game == 20 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_20SEC);
            }
            else if ( seconds_left_in_game == 10 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_10SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }
            /**
            else if ( seconds_left_in_game == 9 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_9SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            } 
            else if ( seconds_left_in_game == 8 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_8SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }
            else if ( seconds_left_in_game == 7 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_7SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            } 
            else if ( seconds_left_in_game == 6 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_6SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }  **/                      
            else if ( seconds_left_in_game == 5 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_5SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            } 
            else if ( seconds_left_in_game == 4 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_4SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }
            else if ( seconds_left_in_game == 3 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_3SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }
            else if ( seconds_left_in_game == 2 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_2SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }           
            else if ( seconds_left_in_game == 1 ){
                sound_play(SND_SOUNDS_ANNOUNCER_ENDS_1SEC);
                FastLED.delay(MSG_WAIT_DELAY_MS);
            }                       
        }
        else{
            //game over
            if ( seconds_since_game_start == clockState->game_duration_secs ){
                sound_play(SND_SOUNDS_0023_ANNOUNCER_VICTORY);
                FastLED.delay(MSG_VICTORY_DELAY_MS);
            }
            clockState->clockState = ClockState::OVER;
            clockState->secs_till_start = 0;
            clockState->time_to_display_secs = 0;
            clockState->game_elapsed_secs=clockState->game_duration_secs;  
            clockState->game_remaining_secs=0;              
        }
    }

}