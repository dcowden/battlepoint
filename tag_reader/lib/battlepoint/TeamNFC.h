#ifndef __INC_NFC_H
#define __INC_NFC_H

typedef enum{
    MEDIC=1,
    RESPAWN=2,
    FLAG=3,
    CONFIG=4
} NFCCardType;

typedef struct { 
    int max_spawns = 100;
    int spawns = 0;
    int max_hp=10;
    int hp = 10;
    int big_hit=3;
    int little_hit=1;
    int invuln_ms=3000;
    long invuln_start=0;
    bool is_dead = false;
    bool is_invul = false;
    bool is_forever_dead = false;
} LifeConfig;

void updateLife(LifeConfig* config){
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

}
void printLift(LifeConfig* config){
  //Log.notice("DMG: ");  Log.notice(config->big_hit);  Log.notice("/"); Log.noticeln(config->little_hit);
  //Log.notice("HP: ");  Log.notice(config->hp);  Log.notice("/"); Log.noticeln(config->max_hp);
  //Log.notice("Lives: "); Log.notice(config->spawns); Log.notice("/"); Log.noticeln(config->max_spawns);
  //Log.notice("Dead: "); if ( config->is_dead){ Log.noticeln("Y");} else {Log.noticeln("N");} 
  //Log.notice("Inv: "); if ( config->is_invul){ Log.noticeln("Y");} else {Log.noticeln("N");}
}

void respawn(LifeConfig* config){
    if ( config->spawns >= config->max_spawns){
        config->spawns += 1;
        config->is_forever_dead = true;
    }
    else{
        config->spawns += 1;
        config->hp = config->max_hp;
        config->is_dead = false;
        config->is_forever_dead = false;
    }
}

void medic(LifeConfig* config, int add_hp){
    config->hp += add_hp;
    if( config->hp > config->max_hp){
        config->hp = config->max_hp;
    }
}

void big_hit(LifeConfig* config){
    config->hp -= config->big_hit;
    if ( config->hp <= 0){
        config->is_dead = true;
        config->hp = 0;
    }
}

void little_hit(LifeConfig* config){
    config->hp -= config->little_hit;
    if ( config->hp <= 0){
        config->is_dead = true;
        config->hp = 0;
    }
}

#endif