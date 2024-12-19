#ifndef __INC_NFC_H
#define __INC_NFC_H


const long START_TIME_NOTSTARTED = -1L;
typedef enum{
    MEDIC=1,
    FLAG=3,
    CLASS=4
} NFCCardType;

typedef enum{
    PC_MEDIC=1,
    PC_HEAVY=2,
    PC_SOLDIER=3,
    PC_SNIPER=4,
    PC_ANONYMOUS=5,
    PC_SCOUT =6,
} PlayerClass;

typedef enum {
    DEAD=0,
    RESPAWNING=1,
    ALIVE=2
} LifeStage;

typedef struct { 

    //class info
    int class_id=PlayerClass::PC_ANONYMOUS;
    int max_hp=3;
    int hp = 3;
    int big_hit=3;
    int little_hit=1;
    long respawn_ms=10000;
    long invuln_ms=5000;

    //state
    long spawn_time = START_TIME_NOTSTARTED;
    long respawn_allowed_time_ms=START_TIME_NOTSTARTED;
    long death_time = START_TIME_NOTSTARTED;
    int state = LifeStage::ALIVE;
    bool respawnRequested = false;
    int maxLifeSpanSecs = START_TIME_NOTSTARTED;
    int respawnCount = 0;
} LifeConfig;

const char * playerClassName( int class_id ){
    if (class_id == PlayerClass::PC_SOLDIER){
        return "Soldier";
    }
    else if ( class_id == PlayerClass::PC_HEAVY){
        return "Heavy";
    }
    else if ( class_id == PlayerClass::PC_MEDIC){
        return "Medic";
    }
    else if ( class_id== PlayerClass::PC_SCOUT){
        return "Scout";
    }
    else if ( class_id == PlayerClass::PC_SNIPER){
        return "Sniper";
    }
    else if ( class_id == PlayerClass::PC_ANONYMOUS){
        return "Anon";
    }    
    else{
        return "None";
    }
}

int secondsBetween( long end, long start){
  return (int)((end - start)/1000);
}

void requestRespawn(LifeConfig &config,long current_time_millis){
    Log.infoln("Respawn Requested");
    config.respawnRequested = true;
}

void _respawn(LifeConfig &config,long current_time_millis){
    config.hp = config.max_hp;
    config.state = LifeStage::ALIVE;
    config.respawnRequested = false;
    config.respawn_allowed_time_ms = START_TIME_NOTSTARTED;
    config.spawn_time = current_time_millis;
    config.respawnCount ++;
}

void _kill(LifeConfig &config, long current_time_millis){
    config.state = LifeStage::DEAD;
    config.hp = 0;
    config.death_time = current_time_millis;

    //important: this is computed here so that even if we change classes, 
    //we can't respawn until the timeout for THIS class
    config.respawn_allowed_time_ms = (current_time_millis + config.respawn_ms);        
    Log.warningln("Player is dead. Respawn allowed in %l ms (at %l)", config.respawn_ms,config.respawn_allowed_time_ms);
    config.respawnRequested = false;    
    int lifeSpanSecs = secondsBetween( current_time_millis,config.spawn_time);
    if ( lifeSpanSecs> config.maxLifeSpanSecs){
        config.maxLifeSpanSecs = lifeSpanSecs;
    }

}
void medic(LifeConfig &config, int add_hp){
    config.hp += add_hp;
    if( config.hp > config.max_hp){
        config.hp = config.max_hp;
    }
}

bool big_hit(LifeConfig &config){
    config.hp -= config.big_hit;
    return config.hp > 0;
}

bool little_hit(LifeConfig &config){
    config.hp -= config.little_hit;
    return config.hp > 0;
}


void updateLife(LifeConfig &config, long current_time_millis){

    if ( config.state == LifeStage::ALIVE  ){
        if ( config.hp <= 0 ){
            _kill(config, current_time_millis);
        }        
    }
    else if ( config.state == LifeStage::DEAD){
        if ( config.respawnRequested ){
            config.state = LifeStage::RESPAWNING;
        }
        else{
            //no respawn requested, so stay dead
        }
    }
    else if ( config.state == LifeStage::RESPAWNING ){
        if ( config.respawn_allowed_time_ms != START_TIME_NOTSTARTED ){
            if (current_time_millis > config.respawn_allowed_time_ms){
                Log.warningln("Respawn Requested, and time is elapsed ( %l > %l ). Triggering Respawn.",current_time_millis, config.respawn_allowed_time_ms);
                _respawn(config,current_time_millis);
                
            }
            else{
                //stay respawning
            } 
        }
    }

    /*
    if ( config->is_invul){
        if (millis() > (config->invuln_start + config->invuln_ms)){
            config->is_invul = false;
        }
    }
    if ( config->hp <= 0 ){
        config->is_dead = true;
        config->is_invul = true;
        config->invuln_start = millis();
    }
    */
}


#endif