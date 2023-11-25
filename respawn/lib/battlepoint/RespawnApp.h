#ifndef __INC_RESPAWNAPP_H
#define __INC_RESPAWNAPP_H
#include <FastLED.h>
#include <RespawnTimer2.h>
#include <NonBlockingRtttl.h>
#include <ArduinoLog.h>

CRGB COLOR_STOP = CRGB::Red;
CRGB COLOR_GO = CRGB::Green;
CRGB COLOR_READY = CRGB::Yellow;
CRGB COLOR_INACTIVE = CRGB::Black;

const char * SOUND_MARIO_1UP = "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * SOUND_MARIO_DEAD = "death:d=32,o=4,b=355:8b.,p.,8f.5,p.,4p,8f.5,p.,4f5,16p,4e5,16p,4d5,16p,8c.5,p.,8e.,p.,4p,8e.,p.,8c.,p.";
const char * SOUND_NO_SLOTS = "StarwarsI:d=16,o=5,b=100:4e,4e,4e,8c,p,g,4e,8c,p,g,4e,4p";

typedef enum {
  CONFIGURE =1,
  RUN =2
} OperationMode;

CRGB getColorForTimerState(RespawnTimerState state){
    if ( state == RespawnTimerState::IMMINENT ){
        return COLOR_READY;
    } 
    else if ( state == RespawnTimerState::FINISHED){
        return COLOR_GO;
    }
    else if ( state == RespawnTimerState::RESPAWNING){
        return COLOR_STOP;
    }
    else {
        return COLOR_INACTIVE;
    }
}

class RespawnPlayer{
    public:
        RespawnPlayer(CRGB* in_leds, uint8_t in_index, int in_soundPin){
            leds = in_leds;
            index = in_index;
            soundPin = in_soundPin;
        };

        bool acceptRespawnRequest(long durationMillis, long currentTimeMillis){
            if ( ! timer->isAvailable(currentTimeMillis)){
                return false;
            }
            else{
                Log.warningln("Starting Timer, Slot %d, Duration=%d", index, durationMillis);
                timer->start(durationMillis,currentTimeMillis);
                Log.noticeln("Beginning Respawn, timer=%d",index);
                rtttl::begin(soundPin, SOUND_MARIO_DEAD);                
                return true;
            }
        };
        void update(long currentTimeMillis){
            RespawnTimerState state = timer->computeTimerState(currentTimeMillis);
            CRGB newColor = getColorForTimerState(state);
            leds[index] = newColor;
            notifyFinished(state);
        };

        void disable(){
            timer->disable();
        };

    protected:
        void notifyFinished(RespawnTimerState state){
            if ( state== RespawnTimerState::FINISHED && ! notifiedFinish){
                notifiedFinish = true;
                Log.noticeln("Respawn, complete timer=%d",index);
                rtttl::begin(soundPin, SOUND_MARIO_1UP);
            }
        }
        CRGB* leds;
        uint8_t index;
        int soundPin;
        RespawnTimer2* timer;
        bool notifiedFinish=false;
};

class RespawnApp{
    public:
        
        RespawnApp(CRGB* leds,int soundPin){
            leds = leds;
            soundPin = soundPin;
            player1 = new RespawnPlayer(leds,0,soundPin);
            player2 = new RespawnPlayer(leds,1,soundPin);
            player3 = new RespawnPlayer(leds,2,soundPin);
            player4 = new RespawnPlayer(leds,3,soundPin);
        };
        OperationMode getCurrentMode(){
            return currentMode;
        };
        void init(){

        };
        void startConfigureRespawnTime(long currentTimeMillis){
            Log.warningln("Entering config mode: disabling timers");
            currentMode = OperationMode::CONFIGURE;
            configuredDurationStartMillis = currentTimeMillis;
            disableTimers();
        };
        long getConfiguredDuration(long currentTimeMillis){
            currentMode = OperationMode::RUN;
            return currentTimeMillis -configuredDurationStartMillis;
        }
        void update(long currentTimeMillis){
            if ( currentMode == OperationMode::RUN){
                player1->update(currentTimeMillis);
                player2->update(currentTimeMillis);
                player3->update(currentTimeMillis);
                player4->update(currentTimeMillis);            
            }        
        };
        void disableTimers(){
            player1->disable();
            player2->disable();
            player3->disable();
            player4->disable();
        };
        void requestRespawn(long durationMillis, long currentTimeMillis){
            if ( currentMode == OperationMode::CONFIGURE){
                Log.warningln("Respawn Request Ignored: another button is long-pressed, so we're in config mode");
            }
            else{
                if ( player1->acceptRespawnRequest(durationMillis,currentTimeMillis )) return;
                if ( player2->acceptRespawnRequest(durationMillis,currentTimeMillis )) return;
                if ( player3->acceptRespawnRequest(durationMillis,currentTimeMillis )) return;
                if ( player4->acceptRespawnRequest(durationMillis,currentTimeMillis )) return;  
                signalNoSlotsAvailable();  
            }                               
        };
        void signalNoSlotsAvailable(){
            Log.warning("No Respawn Slot Available");
            rtttl::begin(soundPin, SOUND_NO_SLOTS);
        };

    protected:
        OperationMode currentMode = OperationMode::RUN;
        long configuredDurationStartMillis = 0;
        CRGB* leds;
        int soundPin;
        RespawnPlayer* player1;
        RespawnPlayer* player2;
        RespawnPlayer* player3;
        RespawnPlayer* player4;
};
#endif