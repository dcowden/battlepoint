#include <TeamNFC.h>
#include <ArduinoLog.h>
#include <Arduino.h>

void setDefaults( NFCCard &card){
    card.type = NFCCardType::UNKNOWN;
}

void setDefaults( ClassCard &card){  
  card.big_hit=1;
  card.little_hit=1;
  card.max_hp=1;
  card.player_class = PlayerClass::PC_ANONYMOUS;
  card.invuln_ms=1000;
  card.respawn_ms=1000;
  card.team = TeamChar::T_NONE;    
}
void setDefaults( MedicCard &card){
    card.hp = 1;    
}

void setDefaults( FlagCard &card){
    card.value = 1;
    card.team = TeamChar::T_NONE;
}

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
    Log.warningln("Adding %d hp.", add_hp);
    adjust_hp(config, add_hp, current_time_millis);    
}

void big_hit(LifeConfig &config, long current_time_millis){
    adjust_hp(config, -config.big_hit, current_time_millis);
}

void little_hit(LifeConfig &config, long current_time_millis){
    adjust_hp(config, -config.little_hit, current_time_millis);    
}