#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>
#include <target.h>

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
MeterSettings get_koth_meter_settings(int hits_to_win){
    MeterSettings s;
    s.center = { .startIndex = 0, .endIndex = 15, .max_val = 100, .val = 0, .fgColor = CRGB::Black, .bgColor = CRGB::Black };
    s.left = { .startIndex = 0, .endIndex = 15, .max_val = hits_to_win, .val = 0, .fgColor = CRGB::Blue, .bgColor = CRGB::Black };   
    s.right = { .startIndex = 0, .endIndex = 15, .max_val = hits_to_win, .val = 0, .fgColor = CRGB::Red, .bgColor = CRGB::Black }; 
    s.leftTop = {.startIndex = 0,.endIndex = 9,.max_val = 10,.val = 10,.fgColor = CRGB::Blue,.bgColor = CRGB::Black };
    s.leftBottom = {.startIndex = 0,.endIndex = 9,.max_val = 10,.val = 10,.fgColor = CRGB::Blue,.bgColor = CRGB::Black };  
    s.rightTop = {.startIndex = 10,.endIndex = 19,.max_val = 10,.val = 10,.fgColor = CRGB::Red,.bgColor = CRGB::Black };
    s.rightBottom = {.startIndex = 10,.endIndex = 19,.max_val = 10,.val = 10,.fgColor = CRGB::Red,.bgColor = CRGB::Black };                                       
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

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        gs.meters = get_koth_meter_settings(settings.hits.to_win);
    }
    return gs;

}
void updateGameHits(GameState* current, SensorState sensors){
    if ( sensors.rightScan.was_hit ) current->hits.red_hits++;
    if ( sensors.leftScan.was_hit ) current->hits.blu_hits++;
}
void updateGame(GameState* current, SensorState sensors, GameSettings settings, Clock* clock){
    
    current->time.last_update_millis = clock->milliseconds();
    updateGameHits(current,sensors);

    int red_hits = current->hits.red_hits;
    int blu_hits = current->hits.blu_hits;


    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        if ( (blu_hits >= settings.hits.to_win ) && (blu_hits > (red_hits + settings.hits.victory_margin)  )  ){
                current->result.winner = Team::BLU;
                current->status = GameStatus::GAME_STATUS_ENDED;
        }else if ( (red_hits >= settings.hits.to_win ) && (red_hits > (blu_hits + settings.hits.victory_margin)  )  ){
                current->result.winner = Team::RED;
                current->status = GameStatus::GAME_STATUS_ENDED;
        }else if ( (red_hits >= settings.hits.to_win) || (red_hits  >= settings.hits.to_win) ){
                current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            current->status = GameStatus::GAME_STATUS_RUNNING;
        }
    }
}


