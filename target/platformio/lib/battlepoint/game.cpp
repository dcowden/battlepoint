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

    //TODO: set up the indexes right once we know the hardware
    MeterSettings s;        
    s.leftTop.startIndex = 0;
    s.leftTop.endIndex=9;
    s.leftTop.max_val=DEFAULT_MAX_VAL;
    s.leftTop.val=0;
    s.leftTop.fgColor=CRGB::Blue;
    s.leftTop.bgColor=CRGB::Black;
    s.leftTop.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.leftTop.last_flash_millis=0;    

    s.leftBottom.startIndex = 0;
    s.leftBottom.endIndex = 9;
    s.leftBottom.max_val = DEFAULT_MAX_VAL;
    s.leftBottom.val = 0;
    s.leftBottom.fgColor = CRGB::Blue;
    s.leftBottom.bgColor = CRGB::Black;
    s.leftBottom.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.leftBottom.last_flash_millis=0;    

    s.rightTop.startIndex = 10;
    s.rightTop.endIndex = 19;
    s.rightTop.max_val = DEFAULT_MAX_VAL;
    s.rightTop.val = 0;
    s.rightTop.fgColor = CRGB::Red;
    s.rightTop.bgColor = CRGB::Black;
    s.rightTop.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.rightTop.last_flash_millis=0;    

    s.rightBottom.startIndex = 10;
    s.rightBottom.endIndex = 19;
    s.rightBottom.max_val = DEFAULT_MAX_VAL;
    s.rightBottom.val = 0;
    s.rightBottom.fgColor = CRGB::Red;
    s.rightBottom.bgColor = CRGB::Black;
    s.rightBottom.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.rightBottom.last_flash_millis=0;         

    s.center.startIndex = 0;
    s.center.endIndex = 16;
    s.center.max_val = DEFAULT_MAX_VAL;
    s.center.val = 0;
    s.center.fgColor = CRGB::Red;
    s.center.bgColor = CRGB::Black;
    s.center.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.center.last_flash_millis=0;

    s.left.startIndex = 0;
    s.left.endIndex = 16;
    s.left.max_val = DEFAULT_MAX_VAL;
    s.left.val = 0;
    s.left.fgColor = CRGB::Red;
    s.left.bgColor = CRGB::Black;
    s.left.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.left.last_flash_millis=0;

    s.right.startIndex = 0;
    s.right.endIndex = 16;
    s.right.max_val = DEFAULT_MAX_VAL;
    s.right.val = 0;
    s.right.fgColor = CRGB::Red;
    s.right.bgColor = CRGB::Black;
    s.right.flash_interval_millis=FlashInterval::FLASH_NONE; 
    s.right.last_flash_millis=0;

    return s;
}

void configureMeter( LedMeter* meter, int max_val, int val, CRGB fg, CRGB bg){
    meter->max_val = max_val;
    meter->val = val;
    meter->fgColor = fg;
    meter->bgColor = bg;   
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
        configureMeter(&gs.meters.center, DEFAULT_MAX_VAL, 0, CRGB::Black, CRGB::Black);   //NOT USED in this mode
        configureMeter(&gs.meters.left, settings.hits.to_win, 0, CRGB::Blue, CRGB::Black); //hits scored for blue , count from zero to total required to win
        configureMeter(&gs.meters.right, settings.hits.to_win, 0, CRGB::Red, CRGB::Black); //hits scored for red , count from zero to total required to win
    }

    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        configureMeter(&gs.meters.center, DEFAULT_MAX_VAL, 0, CRGB::Red, CRGB::Blue);                          // ratio of red to total hits.  max_val gets changed once there are non-zero total hits
        configureMeter(&gs.meters.left, settings.timed.max_duration_seconds, 0, CRGB::Blue, CRGB::Black);      // game progress, count from zero to total game duration
        configureMeter(&gs.meters.right, settings.timed.max_duration_seconds, 0, CRGB::Red, CRGB::Black);      // game progress, count from zero to total game duration
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME ){
        configureMeter(&gs.meters.center, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Blue);       // capture progress: background = owner, foreground: count up from zero to hits required to capture
        configureMeter(&gs.meters.left, settings.timed.ownership_time_seconds, 0, CRGB::Blue, CRGB::Black);  //ownership time for blue, count from zero to time required to win
        configureMeter(&gs.meters.right, settings.timed.ownership_time_seconds, 0, CRGB::Red, CRGB::Black);   //ownership time for red, count from zero to time required to win
    }
    else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND ){
        configureMeter(&gs.meters.center, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Black);   //hit progress: count from zero to hits required to win
        configureMeter(&gs.meters.left, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Black);    //hit progress: count from zero to hits required to win
        configureMeter(&gs.meters.right, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Black);    //hit progress: count from zero to hits required to win
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


