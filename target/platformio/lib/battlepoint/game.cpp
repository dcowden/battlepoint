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
    gs.capture.capture_overtime_seconds = 30;
    gs.capture.hits_to_capture = 10;
    
    gs.timed.max_duration_seconds=120;
    gs.timed.ownership_time_seconds=120;
    gs.timed.max_overtime_seconds=120;

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
    gs.bluHits.hits = 0;
    gs.bluHits.last_decay_millis = 0;
    gs.bluHits.last_hit_millis = 0;
    gs.redHits.hits = 0;
    gs.redHits.last_decay_millis = 0;
    gs.redHits.last_hit_millis = 0;    
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

//TODO: refactor, it's nearly all duplicated!
void updateGameHits(GameState* current, SensorState sensors, long current_time_millis){
    if ( sensors.rightScan.was_hit ){
       current->redHits.hits++;
       current->redHits.last_hit_millis = current_time_millis;
    } 

    if ( sensors.leftScan.was_hit ){
       current->bluHits.hits++;
       current->bluHits.last_hit_millis = current_time_millis;    
    } 
}


//assumes that captureHits is being updated
void applyHitDecay(GameState* current, GameSettings settings, long current_time_millis){
    if ( settings.capture.capture_decay_rate_secs_per_hit > 0 ){
        Log.traceln("Time= %d, Decay Rate Does apply",current_time_millis);
        if(current->ownership.capturing == Team::RED || current->ownership.capturing == Team::BLU ){
            Log.traceln("A Team is Capturing");
            long millis_since_last_decay = (current_time_millis - current->ownership.last_decay_millis);
            long decay_millis = settings.capture.capture_decay_rate_secs_per_hit*1000;
            if ( millis_since_last_decay > decay_millis){
                if ( current->ownership.capture_hits > 0 ){
                    long old_hits = current->ownership.capture_hits;
                    long new_hits = current->ownership.capture_hits - 1;
                    Log.infoln("Applying Decay, %d ->%d",old_hits,new_hits);
                    current->ownership.capture_hits= new_hits;
                    current->ownership.last_decay_millis = current_time_millis;
                }
            }
        }
    }
}

//updates timeExpired based on game state. 
//makes testing easier, so we dont have to set up all those conditions
void updateGameTime(GameState* current,GameSettings settings, long current_time_millis){

    long elapsed_since_start = (current_time_millis - current->time.start_time_millis)/1000;


    bool isExpired = false;
    bool isOvertimeExpired = false;
    if ( settings.timed.max_duration_seconds > 0){
        if ( elapsed_since_start > settings.timed.max_duration_seconds){
            isExpired =  true;
        }
        if ( elapsed_since_start > settings.timed.max_duration_seconds + settings.timed.max_overtime_seconds){
            isOvertimeExpired = true;
        }
    }
    current->time.timeExpired = isExpired;
    current->time.overtimeExpired = isOvertimeExpired;
    
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
    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;
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
        if ( current->time.overtimeExpired ){
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
    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;

    //TODO: update meters all in one place?        
    current->meters.left.meter.val = blu_hits;
    current->meters.right.meter.val = red_hits;
    if ( current->time.overtimeExpired ){
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( current->time.timeExpired ){
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
    int blu_hits = current->bluHits.hits;

    if ( blu_hits >= settings.capture.hits_to_capture){
        endGame(current, Team::BLU);
    }
    else if ( current->time.overtimeExpired){
        endGame(current, Team::RED);
    }
    else if ( current->time.timeExpired ){
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
void triggerOvertime(GameState* current,  GameSettings settings){
    current->ownership.overtime_remaining_millis = settings.capture.capture_overtime_seconds;
}
void capture(GameState* current,  GameSettings settings){
    current->ownership.owner = current->ownership.capturing;
    current->ownership.capturing = Team::NOBODY;        
    current->ownership.capture_hits = 0;
}

void updateOwnership(GameState* current,  GameSettings settings, long current_time_millis){

    long elapsed_since_last_update = (current_time_millis - current->time.last_update_millis);

    if ( current->ownership.owner== Team::RED){
        current->ownership.red_millis += elapsed_since_last_update;
    }

    if ( current->ownership.owner== Team::BLU){
        current->ownership.blu_millis += elapsed_since_last_update;
    }
    Log.info("Checking for Capture, %d/%d to capture.",current->ownership.capture_hits,settings.capture.hits_to_capture);
    if ( current->ownership.capture_hits >= settings.capture.hits_to_capture ){
        if ( current->ownership.owner != Team::NOBODY){
            triggerOvertime(current,settings);
        }
        capture(current,settings);
    }

}

void updateFirstToOwnTimeGame(GameState* current,  GameSettings settings, Clock* clock){
    /*
        VICTORY
        Capturing the point requires a given number of hits.
        Once the point is captured, the owning team accumulates time. 

        The winner is the first team to accumulate the given amount of ownership time, 
        AND survive any contests.

        HITS
        in this game, hits decay at the rate provided by capture settings. 
        defense and offense can both score hits, but offense hits are weighed hire than defense its        

        TIME
        Any time the team owns the point, their time accumulates.
        The first team to accumulate the given amount of ownership time wins, after any overtime contests are resolved.
        The time for a capture overtime are defined separately from the maximum game overtime.

        an overtime contest is triggered when any of these events occur:
           (1) the opposing team beings capturing
           (2) one team passes the game winning time, but the other team has already passed the required too

        If there is a time limit on the game, the team with the most ownership wins, subject to the usual contest rules.
        if the time limit on a game passes, and neither team has any ownership, the game ends in a TIE.

        if the game lasts as long as the maximum overtime limit, then the game ends immediately:
            the team with the most ownership wins
            if that's tied, the team capturing wins
            if nobody has ownership or capture progress, the game ends in a TIE


    */     
    //run for all us already:
    //updateGameTime()
    //updateGameHits()
    applyHitDecay(current, settings,clock->milliseconds());
    updateOwnership(current,settings,clock->milliseconds());

    long ownership_time_to_win_millis = 1000*settings.timed.ownership_time_seconds;
    bool blue_time_complete = (current->ownership.blu_millis > ownership_time_to_win_millis);
    bool red_time_complete = (current->ownership.red_millis > ownership_time_to_win_millis);

    bool isContested  = ( current->ownership.overtime_remaining_millis > 0 );

    if ( blue_time_complete ){
        if ( isContested ){
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            endGame(current,Team::BLU);
        }
    }
    else if ( red_time_complete ){
        if ( isContested ){
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            endGame(current,Team::RED);
        }
    }
    
}

void updateMostOwnInTimeGame(GameState* current,  GameSettings settings,Clock* clock){
    endGame(current, Team::NOBODY);
}

void updateGame(GameState* current, SensorState sensors, GameSettings settings, Clock* clock){
   
    updateGameTime(current, settings, clock->milliseconds());
    updateGameHits(current,sensors,clock->milliseconds());

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        updateFirstToHitsGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME){
        updateMostHitsInTimeGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME){
        updateFirstToOwnTimeGame(current,settings,clock);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND){
        updateAttackDefendGame(current,settings);
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_OWN_IN_TIME){
        updateMostOwnInTimeGame(current,settings,clock);
    }
    else{
        Log.errorln("Unknown Game Type: %d", settings.gameType);
    }

    //very last to make sure everyone can see the time delta during update
    current->time.last_update_millis = clock->milliseconds();
}


