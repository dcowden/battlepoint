#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>
#include <target.h>

#define DEFAULT_MAX_VAL 10

GameSettings DEFAULT_GAMESETTINGS(){
    GameSettings gs;
    gs.hits.to_win = 10;
    gs.hits.victory_margin=1;
    gs.capture.capture_cooldown_seconds=10;
    gs.capture.capture_decay_rate_secs_per_hit=1;
    gs.capture.capture_offense_to_defense_ratio=4;
    gs.capture.hits_to_capture = 10;
    gs.timed.max_duration_seconds=20;
    return gs;
}
const char* getCharForStatus(GameStatus s){
    if ( s == GameStatus::GAME_STATUS_ENDED){
        return "E";
    }
    else if ( s == GameStatus::GAME_STATUS_OVERTIME){
        return "O";
    }
    else{
        return "R";
    }
}
const char* getCharForGameType(GameType t){
    if ( t == GAME_TYPE_ATTACK_DEFEND ){
        return "AD";
    }
    else if ( t == GAME_TYPE_KOTH_FIRST_TO_HITS){
        return "FIRST_HITS";
    }
    else if ( t == GAME_TYPE_KOTH_MOST_HITS_IN_TIME){
        return "MOST_HITS";
    }
    else if ( t == GAME_TYPE_KOTH_MOST_OWN_IN_TIME){
        return "MOST_OWN";
    }
    else{
        return "UNKNOWN";
    }
}

MeterSettings base_meter_settings(){
    MeterSettings s;
    s.leftTop = {.startIndex = 0,.endIndex = 9,.max_val = DEFAULT_MAX_VAL,.val = 0,.fgColor = CRGB::Blue,.bgColor = CRGB::Black };
    s.leftBottom = {.startIndex = 0,.endIndex = 9,.max_val = DEFAULT_MAX_VAL,.val = 0,.fgColor = CRGB::Blue,.bgColor = CRGB::Black };  
    s.rightTop = {.startIndex = 10,.endIndex = 19,.max_val = DEFAULT_MAX_VAL,.val = 0,.fgColor = CRGB::Red,.bgColor = CRGB::Black };
    s.rightBottom = {.startIndex = 10,.endIndex = 19,.max_val = DEFAULT_MAX_VAL,.val = 0,.fgColor = CRGB::Red,.bgColor = CRGB::Black };                   
    return s;
}

GameState startGame(GameSettings settings, Clock* clock){
    GameState gs;
    
    gs.time.start_time_millis = clock->milliseconds();
    gs.hits.blu_hits = 0;
    gs.hits.red_hits = 0;
    gs.status = GameStatus::GAME_STATUS_RUNNING;
    gs.result.winner = Team::NOBODY;
    gs.ownership.blu_millis = 0;
    gs.ownership.red_millis = 0;
    gs.ownership.owner = Team::NOBODY;
    gs.ownership.capturing = Team::NOBODY;
    gs.ownership.capture_hits = 0;
    gs.meters = base_meter_settings();

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        gs.meters.center = { .startIndex = 0, .endIndex = 15, .max_val = DEFAULT_MAX_VAL, .val = 0, .fgColor = CRGB::Black, .bgColor = CRGB::Black };
        gs.meters.left = { .startIndex = 0, .endIndex = 15, .max_val = settings.hits.to_win, .val = 0, .fgColor = CRGB::Blue, .bgColor = CRGB::Black };   
        gs.meters.right = { .startIndex = 0, .endIndex = 15, .max_val = settings.hits.to_win, .val = 0, .fgColor = CRGB::Red, .bgColor = CRGB::Black }; 
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        MeterSettings s;
        gs.meters.center = { .startIndex = 0, .endIndex = 15, .max_val = DEFAULT_MAX_VAL, .val = 0, .fgColor = CRGB::Black, .bgColor = CRGB::Black };
        gs.meters.left = { .startIndex = 0, .endIndex = 15, .max_val = DEFAULT_MAX_VAL, .val = 0, .fgColor = CRGB::Blue, .bgColor = CRGB::Black };   
        gs.meters.right = { .startIndex = 0, .endIndex = 15, .max_val = DEFAULT_MAX_VAL, .val = 0, .fgColor = CRGB::Red, .bgColor = CRGB::Black }; 
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME ){
        MeterSettings s;
        gs.meters.center = { .startIndex = 0, .endIndex = 15, .max_val = settings.capture.hits_to_capture, .val = 0, .fgColor = CRGB::Black, .bgColor = CRGB::Black };
        gs.meters.left = { .startIndex = 0, .endIndex = 15, .max_val = settings.timed.ownership_time_seconds, .val = 0, .fgColor = CRGB::Blue, .bgColor = CRGB::Black };   
        gs.meters.right = { .startIndex = 0, .endIndex = 15, .max_val = settings.timed.ownership_time_seconds, .val = 0, .fgColor = CRGB::Red, .bgColor = CRGB::Black }; 
    }
    else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND ){
        MeterSettings s;
        gs.meters.center = { .startIndex = 0, .endIndex = 15, .max_val = settings.capture.hits_to_capture, .val = 0, .fgColor = CRGB::Black, .bgColor = CRGB::Black };
        gs.meters.left = { .startIndex = 0, .endIndex = 15, .max_val = settings.capture.hits_to_capture, .val = 0, .fgColor = CRGB::Blue, .bgColor = CRGB::Black };   
        gs.meters.right = { .startIndex = 0, .endIndex = 15, .max_val = settings.capture.hits_to_capture, .val = 0, .fgColor = CRGB::Red, .bgColor = CRGB::Black }; 
    }    
    else{
        Serial.println("UNKNOWN GAME TYPE");
    }
    return gs;

}
void updateGameHits(GameState* current, SensorState sensors){
    if ( sensors.rightScan.was_hit ) current->hits.red_hits++;
    if ( sensors.leftScan.was_hit ) current->hits.blu_hits++;
}

bool isGameTimedOut(GameState* current,GameSettings settings, Clock* clock){
    current->time.last_update_millis = clock->milliseconds();
    long elapsed = (clock->milliseconds() - current->time.start_time_millis)/1000;

    int game_secs = settings.timed.max_duration_seconds;
    if ( game_secs > 0){
        if ( elapsed > game_secs){
            return true;
        }
        else{
            return false;
        }
    }
    return false;
}


void updateGame(GameState* current, SensorState sensors, GameSettings settings, Clock* clock){
    
    bool timedOut = isGameTimedOut(current, settings, clock);
    updateGameHits(current,sensors);

    int red_hits = current->hits.red_hits;
    int blu_hits = current->hits.blu_hits;        

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){

        current->meters.left.val = blu_hits;
        current->meters.right.val = red_hits;

        if ( (blu_hits >= settings.hits.to_win ) && (blu_hits > (red_hits + settings.hits.victory_margin)  )  ){
            current->result.winner = Team::BLU;
            current->status = GameStatus::GAME_STATUS_ENDED;
        }else if ( (red_hits >= settings.hits.to_win ) && (red_hits > (blu_hits + settings.hits.victory_margin)  )  ){
            current->result.winner = Team::RED;
            current->status = GameStatus::GAME_STATUS_ENDED;
        }else if ( (red_hits >= settings.hits.to_win) || (red_hits  >= settings.hits.to_win) ){
            if ( timedOut ){
                current->status = GameStatus::GAME_STATUS_ENDED;
                if ( blu_hits > red_hits){
                    current->result.winner = Team::BLU;
                }
                else if (red_hits > blu_hits ){
                    current->result.winner = Team::RED;
                }
                else{
                    current->result.winner = Team::NOBODY;
                }
            }
            else{
                current->status = GameStatus::GAME_STATUS_OVERTIME;
            }
        }
        else{
            current->status = GameStatus::GAME_STATUS_RUNNING;    
        }
    }
}


