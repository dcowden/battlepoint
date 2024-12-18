#pragma once
#include "NFCCards.h"

typedef enum {
    DEAD=0,
    RESPAWNING=1,
    ALIVE=2,
    INVULNERABLE=3,
    INIT
} LifeStage;

typedef struct { 
    //TODO: divide into a class or nest these?
    //configurations
    int max_hp;
    int big_hit=NOT_SET;
    int little_hit=NOT_SET;
    int invuln_ms=NOT_SET;
    long respawn_ms=NOT_SET;
    int team = TeamChar::T_NONE;
    int player_class = PlayerClass::PC_ANONYMOUS;

    //state
    long respawn_allowed_time_ms=START_TIME_NOTSTARTED;
    long invuln_end_time_ms = START_TIME_NOTSTARTED;
    int hp = 0;
    int state = LifeStage::INIT;    
    bool respawnRequested = false; 

} LifeConfig;

const char * playerClassName( int player_class );
void trackerInit ( LifeConfig &config, long current_timeMillis);
void trackerRequestRespawn(LifeConfig &config , long current_time_millis);
void trackerStartInvuln(LifeConfig &config , long current_time_millis);
void trackerEndInvuln(LifeConfig &config , long current_time_millis);
void trackerUpdateLifeModel(LifeConfig &config, long current_time_millis);
const char * trackerLifeStatus( LifeConfig &config );
void trackerApplyMedic(LifeConfig &config, int add_hp, long current_time_millis);
void trackerBigHit(LifeConfig &config, long current_time_millis);
void trackerLittleHit(LifeConfig &config, long current_time_millis);