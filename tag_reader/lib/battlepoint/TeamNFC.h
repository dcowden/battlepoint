#ifndef __INC_NFC_H
#define __INC_NFC_H

const long START_TIME_NOTSTARTED = -1;
const int NOT_SET = 1;

typedef enum{
    MEDIC=1,
    RESPAWN=2,
    FLAG=3,
    CONFIG=4
} NFCCardType;

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
    int max_hp=NOT_SET;
    int big_hit=NOT_SET;
    int little_hit=NOT_SET;
    int invuln_ms=NOT_SET;
    long respawn_ms=NOT_SET;


    //state
    long respawn_allowed_time_ms=START_TIME_NOTSTARTED;
    long invuln_end_time_ms = START_TIME_NOTSTARTED;
    int hp = 0;
    int state = LifeStage::INIT;    
    bool respawnRequested = false; 

} LifeConfig;


void doRespawn( LifeConfig &config , long current_time_millis){
    config.hp = config.max_hp;
    config.state = LifeStage::ALIVE;
    config.invuln_end_time_ms = START_TIME_NOTSTARTED;
    config.respawnRequested = false;
}

void doKill( LifeConfig &config , long current_time_millis){
    config.state = LifeStage::DEAD;
    config.hp = 0;

    //important: this is computed here so that even if we change classes, 
    //we can't respawn until the timeout for THIS class
    config.respawn_allowed_time_ms = (current_time_millis + config.respawn_ms);        
    config.respawnRequested = false;
}

void requestRespawn(LifeConfig &config , long current_time_millis){
    config.respawnRequested = true;
    if ( config.state == LifeStage::INIT){
       doKill(config, current_time_millis);
    }
}

void start_invuln(LifeConfig &config , long current_time_millis){
    config.state = LifeStage::INVULNERABLE;
    config.invuln_end_time_ms = current_time_millis + config.invuln_ms;
}

void end_invuln(LifeConfig &config , long current_time_millis){
    config.state = LifeStage::ALIVE;
    config.invuln_end_time_ms = START_TIME_NOTSTARTED;
}

void updateLifeStatus(LifeConfig &config, long current_time_millis){
    if ( config.state == LifeStage::INVULNERABLE){
        if (current_time_millis > config.invuln_end_time_ms){
            end_invuln(config,current_time_millis);
        }
    }

    if ( config.state == LifeStage::INIT ){
        //dont do anything: waiting to get configured
    }
    else if ( config.state == LifeStage::ALIVE  ){
        if ( config.hp <= 0 ){
            doKill(config,current_time_millis);
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
        if (current_time_millis > config.respawn_allowed_time_ms){
            doRespawn(config,current_time_millis);
        }
        else{
            //stay respawning
        }     
    }
}

const char * lifeStatus( LifeConfig &config ){
    if ( config.state == LifeStage::RESPAWNING){
        return "RESP";
    }
    else if ( config.state == LifeStage::DEAD){
        return "DEAD";
    }
    else if ( config.state == LifeStage::INVULNERABLE){
        return "INV";
    }
    else if ( config.state == LifeStage::ALIVE){
        return "ALIV";
    }
    else{
        return "UNKN";
    }
}

void logLife(LifeConfig &config){
  Log.noticeln("DMG: %d/%d",config.big_hit, config.little_hit);
  Log.noticeln("HP: %d/%d, config.hp, config.max_hp");
  Log.noticeln("STS: %s", lifeStatus(config)); 
}


void adjust_hp(LifeConfig &config, int delta,long current_time_millis){
    if ( config.state == LifeStage::ALIVE){
        config.hp += delta; 
        config.hp = constrain(config.hp,0,config.max_hp);
        updateLifeStatus(config,current_time_millis);
    }
    else{
        Log.warningln("Can't add health when not alive.");
    }
}

void medic(LifeConfig &config, int add_hp, long current_time_millis){
    adjust_hp(config, add_hp, current_time_millis);    
}

void big_hit(LifeConfig &config, long current_time_millis){
    adjust_hp(config, -config.big_hit, current_time_millis);
}

void little_hit(LifeConfig &config, long current_time_millis){
    adjust_hp(config, -config.little_hit, current_time_millis);    
}

#endif