#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>
#include <target.h>
#include <LedMeter.h>
#include <ArduinoLog.h>

GameSettings DEFAULT_GAMESETTINGS(){
    GameSettings gs;
    gs.BP_VERSION = BP_CURRENT_SETTINGS_VERSION;
    gs.hits.to_win = 10;
    gs.hits.victory_margin=1;
    gs.gameType = GameType::GAME_TYPE_UNSELECTED;
    gs.capture.capture_cooldown_seconds=10;
    gs.capture.capture_decay_rate_secs_per_hit=1;
    gs.capture.capture_offense_to_defense_ratio=4;
    gs.capture.hits_to_capture = 10;
    
    gs.timed.max_duration_seconds=120;
    gs.timed.ownership_time_seconds=120;
    gs.timed.max_overtime_seconds=30;

    gs.target.hit_energy_threshold = 100;
    gs.target.last_hit_millis=0;
    gs.target.trigger_threshold=1000;
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
    initMeter(&s.leftTop.meter,0,9);
    initMeter(&s.leftBottom.meter,0,9);
    initMeter(&s.rightTop.meter,10,19);
    initMeter(&s.rightBottom.meter,10,19);
    initMeter(&s.center.meter,0,15);
    initMeter(&s.left.meter,0,15);
    initMeter(&s.right.meter,0,15);
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
        configureMeter(&gs.meters.center.meter, DEFAULT_MAX_VAL, 0, CRGB::Black, CRGB::Black);   //NOT USED in this mode
        configureMeter(&gs.meters.left.meter, settings.hits.to_win, 0, CRGB::Blue, CRGB::Black); //hits scored for blue , count from zero to total required to win
        configureMeter(&gs.meters.right.meter, settings.hits.to_win, 0, CRGB::Red, CRGB::Black); //hits scored for red , count from zero to total required to win
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        configureMeter(&gs.meters.center.meter, DEFAULT_MAX_VAL, 0, CRGB::Red, CRGB::Blue);                          // ratio of red to total hits.  max_val gets changed once there are non-zero total hits
        configureMeter(&gs.meters.left.meter, settings.timed.max_duration_seconds, 0, CRGB::Blue, CRGB::Black);      // game progress, count from zero to total game duration
        configureMeter(&gs.meters.right.meter, settings.timed.max_duration_seconds, 0, CRGB::Red, CRGB::Black);      // game progress, count from zero to total game duration
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME ){
        configureMeter(&gs.meters.center.meter, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Blue);       // capture progress: background = owner, foreground: count up from zero to hits required to capture
        configureMeter(&gs.meters.left.meter, settings.timed.ownership_time_seconds, 0, CRGB::Blue, CRGB::Black);  //ownership time for blue, count from zero to time required to win
        configureMeter(&gs.meters.right.meter, settings.timed.ownership_time_seconds, 0, CRGB::Red, CRGB::Black);   //ownership time for red, count from zero to time required to win
    }
    else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND ){
        configureMeter(&gs.meters.center.meter, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Black);   //hit progress: count from zero to hits required to win
        configureMeter(&gs.meters.left.meter, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Black);    //hit progress: count from zero to hits required to win
        configureMeter(&gs.meters.right.meter, settings.capture.hits_to_capture, 0, CRGB::Red, CRGB::Black);    //hit progress: count from zero to hits required to win
    }    
    else{
       Log.errorln("UNKNOWN GAME TYPE");
    }
    return gs;

}
void updateGameHits(GameState* current, SensorState sensors){
    if ( sensors.rightScan.was_hit ) current->hits.red_hits++;
    if ( sensors.leftScan.was_hit ) current->hits.blu_hits++;
}

//updates timeExpired based on game state. 
//makes testing easier, so we dont have to set up all those conditions
void updateGameTime(GameState* current,GameSettings settings, Clock* clock){
    current->time.last_update_millis = clock->milliseconds();
    long elapsed = (clock->milliseconds() - current->time.start_time_millis)/1000;

    
    bool isExpired = false;
    bool isOvertimeExpired = false;
    if ( settings.timed.max_duration_seconds > 0){
        if ( elapsed > settings.timed.max_duration_seconds){
            isExpired =  true;
        }
        if ( elapsed > settings.timed.max_duration_seconds + settings.timed.max_overtime_seconds){
            isOvertimeExpired = true;
        }
    }
    current->timeExpired = isExpired;
    current->overtimeExpired = isOvertimeExpired;
}

void endGame( GameState* current, Team winner){
    current->result.winner = winner;
    current->status = GameStatus::GAME_STATUS_ENDED;
}

void endGameWithMostHits(GameState* current, int red_hits, int blu_hits ){
    if ( blu_hits > red_hits){
        endGame(current, Team::BLU);
    }
    else if (red_hits > blu_hits ){
        endGame(current, Team::RED);
    }
    else{
        endGame(current, Team::TIE);
    }    
}

void updateFirstToHitsGame(GameState* current,  GameSettings settings){

    /*

        VICTORY
        Reach the target number of hits, AND more hits than the other team by the victory margin
        If the overtime period does not produce a victory, the game ends in a tie

        HITS
        in this game, hits do not decay. Each team's hits are tracked separately

        TIME
        Overtime begins if a team has reached the target hit count, but is not above the other team by the 
        victory margin, OR if time expires but overtime has not expired

        
        If the teams are tied after the overtime period, the game ends in a tie
        
    */
    int red_hits = current->hits.red_hits;
    int blu_hits = current->hits.blu_hits;
    Log.traceln("RedHits=%d, BluHits=%d",red_hits,blu_hits);

    //TODO: update meters all in one place?        
    current->meters.left.meter.val = blu_hits;
    current->meters.right.meter.val = red_hits;

    //TODO: how to factor out this threshold code? 
    if ( (blu_hits >= settings.hits.to_win ) && (blu_hits >= (red_hits + settings.hits.victory_margin)  )  ){
        endGame(current, Team::BLU);
    }else if ( (red_hits >= settings.hits.to_win ) && (red_hits >= (blu_hits + settings.hits.victory_margin)  )  ){
        endGame(current, Team::RED);
    }else if ( (red_hits >= settings.hits.to_win) || (red_hits  >= settings.hits.to_win) ){                
        //in this game you're in overtime if time is expired OR 
        //one team has reached the target but has not exceeded victory margin
        if ( current->overtimeExpired ){
            endGameWithMostHits(current,red_hits, blu_hits);
        }
        else{
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
    }
    else{
        current->status = GameStatus::GAME_STATUS_RUNNING;    
    }
    

}

void updateMostHitsInTimeGame(GameState* current,  GameSettings settings){
    /*
        VICTORY
        Acheive more hits than the other time in the specified time, and win by the victory margin

        HITS
        in this game, hits do not decay. Each team's hits are tracked separately


        TIMEOUT
        If time up, but neither team is ahead by the victory margin, overtime
        If the teams are tied after overtime, the result is a TIE
        
    */
    int red_hits = current->hits.red_hits;
    int blu_hits = current->hits.blu_hits;

    //TODO: update meters all in one place?        
    current->meters.left.meter.val = blu_hits;
    current->meters.right.meter.val = red_hits;
    if ( current->overtimeExpired ){
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( current->timeExpired ){
        //TODO: how to factor out this threshold code? 
        if ( (blu_hits >= settings.hits.to_win ) && (blu_hits >= (red_hits + settings.hits.victory_margin)  )  ){
            endGame(current, Team::BLU);
        }else if ( (red_hits >= settings.hits.to_win ) && (red_hits >= (blu_hits + settings.hits.victory_margin)  )  ){
            endGame(current, Team::RED);
        }
        else{
            current->status = GameStatus::GAME_STATUS_OVERTIME;    
        }
    }
    else{
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }

}
void updateFirstToOwnTimeGame(GameState* current,  GameSettings settings){
    endGame(current, Team::NOBODY);
}
void updateAttackDefendGame(GameState* current,  GameSettings settings){
    /*
        VICTORY
        Blue is the attacking team, and wins if it achieves the desired number of hits
        within the time alotted.

        HITS
        in this game, hits decay at the rate provided by capture settings.

        TIME
        Overtime is triggered if time is expired, but the attacking team
         is within victory margin of getting the desired number of hits.
        If time and overtime expires, the defending team (red) wins 
    */    
    int blu_hits = current->hits.blu_hits;

    if ( blu_hits >= settings.capture.hits_to_capture){
        endGame(current, Team::BLU);
    }
    else if ( current->overtimeExpired){
        endGame(current, Team::RED);
    }
    else if ( current->timeExpired ){
        if ( blu_hits >= (settings.capture.hits_to_capture - settings.hits.victory_margin)){
             current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            endGame(current,Team::RED);
        }
    }
    else {
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }   
}

void updateMostOwnInTimeGame(GameState* current,  GameSettings settings){
    endGame(current, Team::NOBODY);
}

void updateGame(GameState* current, SensorState sensors, GameSettings settings, Clock* clock){
   
    updateGameTime(current, settings, clock);
    updateGameHits(current,sensors);

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        updateFirstToHitsGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME){
        updateMostHitsInTimeGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME){
        updateFirstToOwnTimeGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND){
        updateAttackDefendGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_OWN_IN_TIME){
        updateMostOwnInTimeGame(current,settings);
    }
    else{
        Log.errorln("Unknown Game Type: %d", settings.gameType);
    }
}


