#include <RespawnUX.h>
#include <RespawnTimer.h>
#include <FastLED.h>
#include <NonBlockingRtttl.h>
#include <ArduinoLog.h>

CRGB COLOR_STOP = CRGB::Red;
CRGB COLOR_GO = CRGB::Green;
CRGB COLOR_READY = CRGB::Yellow;
CRGB COLOR_INACTIVE = CRGB::Black;

const char * SOUND_MARIO_1UP = "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * SOUND_MARIO_DEAD = "death:d=32,o=4,b=355:8b.,p.,8f.5,p.,4p,8f.5,p.,4f5,16p,4e5,16p,4d5,16p,8c.5,p.,8e.,p.,4p,8e.,p.,8c.,p.";
const char * SOUND_NO_SLOTS = "StarwarsI:d=16,o=5,b=100:4e,4e,4e,8c,p,g,4e,8c,p,g,4e,4p";

void signalNoSlotsAvailable(int soundPin){
    rtttl::begin(soundPin, SOUND_NO_SLOTS);
}
void initRespawnUX(RespawnUX* respawnUX, CRGB* leds, uint8_t index,int soundPin){
    respawnUX->index = index;
    respawnUX->leds = leds;
    respawnUX->notifiedFinish=false;
    respawnUX->notifiedStart=false;
    respawnUX->soundPin = soundPin;
}

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

void updateRespawnUX(RespawnUX* respawnUX, RespawnTimerState state, long currentTimeMillis){
    CRGB newColor = getColorForTimerState(state);
    respawnUX->leds[respawnUX->index] = newColor;
    if ( state== RespawnTimerState::RESPAWNING && ! respawnUX->notifiedStart){
        Log.noticeln("Beginning Respawn, timer=%d",respawnUX->index);
        rtttl::begin(respawnUX->soundPin, SOUND_MARIO_DEAD);
        respawnUX->notifiedStart = true;
    }
    if ( state== RespawnTimerState::FINISHED && ! respawnUX->notifiedFinish){
        respawnUX->notifiedFinish = true;
        Log.noticeln("Respawn, complete timer=%d",respawnUX->index);
        rtttl::begin(respawnUX->soundPin, SOUND_MARIO_1UP);
    }
};

