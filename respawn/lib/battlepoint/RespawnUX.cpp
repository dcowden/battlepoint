#include <RespawnUX.h>
#include <RespawnTimer.h>
#include <FastLED.h>
#include <NonBlockingRtttl.h>
#include <ArduinoLog.h>

CRGB COLOR_STOP = CRGB::Red;
CRGB COLOR_GO = CRGB::Green;
CRGB COLOR_READY = CRGB::Yellow;
CRGB COLOR_INACTIVE = CRGB::Black;
const char * SOUND_TETRIS = "tetris:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a,8p,d6,8f6,a6,8g6,8f6,e6,8e6,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,a";
const char * SOUND_ARKNOID = "Arkanoid:d=4,o=5,b=140:8g6,16p,16g.6,2a#6,32p,8a6,8g6,8f6,8a6,2g6";
const char * SOUND_MARIO = "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
const char * SOUND_MARIO_1UP = "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * SOUND_MARIO_DEAD = "death:d=32,o=4,b=355:8b.,p.,8f.5,p.,4p,8f.5,p.,4f5,16p,4e5,16p,4d5,16p,8c.5,p.,8e.,p.,4p,8e.,p.,8c.,p.";

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

