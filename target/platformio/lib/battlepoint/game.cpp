#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>
#include <target.h>

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

    MeterSettings s;
    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){

        //configure meters
        s.center = { .startIndex = 0, .endIndex = 15, .max_val = 100, .val = 0, .fgColor = CRGB::Black, .bgColor = CRGB::Black };
        s.left = { .startIndex = 0, .endIndex = 15, .max_val = settings.hits.to_win, .val = 0, .fgColor = CRGB::Blue, .bgColor = CRGB::Black };   
        s.right = { .startIndex = 0, .endIndex = 15, .max_val = settings.hits.to_win, .val = 0, .fgColor = CRGB::Red, .bgColor = CRGB::Black }; 
        s.leftTop = {.startIndex = 0,.endIndex = 9,.max_val = 10,.val = 10,.fgColor = CRGB::Blue,.bgColor = CRGB::Black };
        s.leftBottom = {.startIndex = 0,.endIndex = 9,.max_val = 10,.val = 10,.fgColor = CRGB::Blue,.bgColor = CRGB::Black };  
        s.rightTop = {.startIndex = 10,.endIndex = 19,.max_val = 10,.val = 10,.fgColor = CRGB::Red,.bgColor = CRGB::Black };
        s.rightBottom = {.startIndex = 10,.endIndex = 19,.max_val = 10,.val = 10,.fgColor = CRGB::Red,.bgColor = CRGB::Black };                                       

    }
    gs.meters = s;
    return gs;

}

GameState updateGame(GameState current, SensorState sensors, GameSettings settings, Clock* clock){
    GameState r = current;
    r.time.last_update_millis = clock->milliseconds();

    if ( sensors.rightScan.was_hit ) r.hits.red_hits++;
    if ( sensors.leftScan.was_hit ) r.hits.blu_hits++;

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        if ( (current.hits.blu_hits >= settings.hits.to_win ) && (current.hits.blu_hits > (current.hits.red_hits + settings.hits.victory_margin)  )  ){
                r.result.winner = Team::BLU;
                r.status = GameStatus::GAME_STATUS_ENDED;
        }else if ( (current.hits.red_hits >= settings.hits.to_win ) && (current.hits.red_hits > (current.hits.blu_hits + settings.hits.victory_margin)  )  ){
                r.result.winner = Team::RED;
                r.status = GameStatus::GAME_STATUS_ENDED;
        }else if ( (current.hits.red_hits >= settings.hits.to_win) || (current.hits.red_hits  >= settings.hits.to_win) ){
                r.status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            r.status = GameStatus::GAME_STATUS_RUNNING;
        }
    }
    return r;
}


