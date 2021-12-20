#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>
#include <target.h>
#include <LedMeter.h>
#include <ArduinoLog.h>


void setDefaultGameSettings(GameSettings* gs){

    gs->hits.to_win = 1;
    gs->hits.victory_margin=1;
    gs->gameType = GameType::GAME_TYPE_UNSELECTED;
    gs->capture.capture_cooldown_seconds=10;
    gs->capture.capture_decay_rate_secs_per_hit=1;
    gs->capture.capture_overtime_seconds = 30;
    gs->capture.hits_to_capture = 10;
    
    gs->timed.max_duration_seconds=120;
    gs->timed.ownership_time_seconds=120;
    gs->timed.max_overtime_seconds=60;

    gs->target.hit_energy_threshold = 5000;
    gs->target.last_hit_millis=0;
    gs->target.trigger_threshold=100;

}

const char* getCharForStatus(GameStatus s){
    if ( s == GameStatus::GAME_STATUS_ENDED){
        return "E";
    }
    else if ( s == GameStatus::GAME_STATUS_OVERTIME){
        return "O";
    }
    else if ( s == GameStatus::GAME_STATUS_PREGAME){
        return "P";
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

Team getWinnerByHitsWithVictoryMargin(int red_hits, int blu_hits, int hits_to_win, int victory_margin){
    bool redWins = (red_hits >= hits_to_win ) && (red_hits >= (blu_hits + victory_margin));
    bool bluWins = (blu_hits >= hits_to_win ) && (blu_hits >= (red_hits + victory_margin));
    if ( redWins){
        return Team::RED;
    }    
    else if ( bluWins){
        return Team::BLU;
    }
    else{
        return Team::NOBODY;
    }
}
Team getWinnerByHitsWithoutVictoryMargin(int red_hits, int blu_hits, int hits_to_win){
    if ( red_hits >= hits_to_win){
        return Team::RED;
    }
    else if ( blu_hits >= hits_to_win){
        return Team::BLU;
    }
    else{
        return Team::NOBODY;
    }
}

void updateLeds(MeterSettings* meters, long current_time_millis ){
  updateLedMeter(meters->left,current_time_millis);
  updateLedMeter(meters->right,current_time_millis);
  updateLedMeter(meters->center,current_time_millis);
  updateLedMeter(meters->leftTop,current_time_millis);
  updateLedMeter(meters->leftBottom,current_time_millis);
  updateLedMeter(meters->rightTop,current_time_millis);
  updateLedMeter(meters->rightBottom,current_time_millis);
}

void setFlashMeterForTeam(Team t, MeterSettings* meters, FlashInterval fi ){
    if ( t == Team::RED ){
        meters->leftBottom->flash_interval_millis = fi;
        meters->leftTop->flash_interval_millis = fi;        
    }
    else  if ( t == Team::BLU ){
        meters->rightBottom->flash_interval_millis = fi;
        meters->rightTop->flash_interval_millis = fi;        
    }
    else{
        Log.warningln("Ignored updating flash on meters for invalid team.");
    }
}
void setMetersToFlashInterval(MeterSettings* meters, long flashInterval){
    meters->center->flash_interval_millis = flashInterval;
    meters->leftBottom->flash_interval_millis = flashInterval;
    meters->leftTop->flash_interval_millis = flashInterval;
    meters->left->flash_interval_millis = flashInterval;
    meters->rightBottom->flash_interval_millis = flashInterval;
    meters->rightTop->flash_interval_millis = flashInterval;
    meters->right->flash_interval_millis = flashInterval;
}

void updateMetersForRunningGame(GameState* game, GameSettings* settings, MeterSettings* meters){
    if ( settings->gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );  
        setMeterValues( meters->center, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Black, CRGB::Black ); 
        setMeterValues( meters->left, game->redHits.hits, settings->hits.to_win, CRGB::Red, CRGB::Black );
        setMeterValues( meters->right, game->bluHits.hits, settings->hits.to_win, CRGB::Blue, CRGB::Black );
    }
    else if ( settings->gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){

        setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );  
        setMeterValues( meters->left, game->time.elapsed_secs, settings->timed.max_duration_seconds, CRGB::Red, CRGB::Black );
        setMeterValues( meters->right, game->time.elapsed_secs, settings->timed.max_duration_seconds, CRGB::Blue, CRGB::Black );

        int total_hits = game->bluHits.hits + game->redHits.hits;
        if ( total_hits  > 0 ){
            setMeterValues(meters->center, game->redHits.hits, total_hits, CRGB::Red, CRGB::Blue);
            setMeterValues(meters->left, game->redHits.hits, total_hits, CRGB::Red, CRGB::Blue);
            setMeterValues(meters->right, game->bluHits.hits, total_hits, CRGB::Blue, CRGB::Red);
        }
        else{
            setMeterValues(meters->center, game->bluHits.hits, total_hits, CRGB::Red, CRGB::Blue);
            meters->center->max_val = 1;
            meters->center->val = 0;
            meters->center->bgColor = CRGB::Black;
            meters->center->fgColor = CRGB::Black;        
        }

    }
    else if ( settings->gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME ){
        //update meters
        int secs_to_win = settings->timed.ownership_time_seconds;
        int red_own_secs = game->ownership.red_millis/1000;
        int blue_own_secs = game->ownership.blu_millis/1000;

        setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black ); 
        setMeterValues( meters->left, red_own_secs, secs_to_win, CRGB::Red, CRGB::Black );
        setMeterValues( meters->right, blue_own_secs, secs_to_win, CRGB::Blue, CRGB::Black ); 

        //the center meter is the capture meter
        //its neutral until someone is the owner. then its the capture meter
        //TODO: this can be factored to be shorter
        if ( game->ownership.owner == Team::NOBODY){
            //if only one team has capture hits, show progress towards capture.
            //if both teams have capture hits, one team must get more than the other by the capture_hits margin.
            int blu_hits = game->bluHits.hits;
            int red_hits = game->redHits.hits;
            if ( blu_hits == red_hits){
                setMeterValues( meters->center, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Yellow, CRGB::Yellow ); 
            }
            else{
                int hit_diff = abs(blu_hits - red_hits);
                if ( blu_hits > red_hits){
                    setMeterValues( meters->center, hit_diff, settings->capture.hits_to_capture, CRGB::Blue, CRGB::Yellow ); 
                }
                else{ //red_hits > blu_hits
                    setMeterValues( meters->center, hit_diff, settings->capture.hits_to_capture, CRGB::Red, CRGB::Yellow ); 
                }
            }
        }
        else if ( game->ownership.owner == Team::BLU) {
            setMeterValues( meters->center, game->ownership.capture_hits, settings->capture.hits_to_capture, CRGB::Red, CRGB::Blue );  
        }
        else if ( game->ownership.owner == Team::RED){
            setMeterValues( meters->center, game->ownership.capture_hits, settings->capture.hits_to_capture, CRGB::Blue, CRGB::Red );  
        }

    }
    else if ( settings->gameType == GameType::GAME_TYPE_ATTACK_DEFEND ){
        int total_hits = game->ownership.capture_hits;        
        int hits_to_capture = settings->capture.hits_to_capture;

        setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->right, total_hits, hits_to_capture, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->left, total_hits, hits_to_capture, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->center, total_hits, hits_to_capture, CRGB::Blue, CRGB::Black );

    }
    else if ( settings->gameType == GameType::GAME_TYPE_TARGET_TEST ){
        int blu_hits = game->bluHits.hits;
        int red_hits = game->redHits.hits;

        const int WARNING_HITS_LEFT = 4;

        int close_to_winning_hits = (settings->hits.to_win - WARNING_HITS_LEFT);

        if ( red_hits > close_to_winning_hits ){
            meters->left->flash_interval_millis = FlashInterval::FLASH_SLOW;
        }
        else{
            meters->left->flash_interval_millis = FlashInterval::FLASH_NONE;
        }

        if ( blu_hits > close_to_winning_hits ){
            meters->right->flash_interval_millis = FlashInterval::FLASH_SLOW;
        }        
        else{
            meters->right->flash_interval_millis = FlashInterval::FLASH_NONE;
        }

        setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
        setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
        setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black ); 
        setMeterValues( meters->left, red_hits, settings->hits.to_win, CRGB::Red, CRGB::Black );
        setMeterValues( meters->right, blu_hits, settings->hits.to_win, CRGB::Blue, CRGB::Black );
    
        //compute the seesaw meter
        //if there are hits, it's the ratio of blue to red
        int total_hits = blu_hits + red_hits;
        if ( total_hits > 0 ){
            float percentRed = (float)red_hits / (float)total_hits * 100.0;
            setMeterValues( meters->center, (int)(percentRed), 100, CRGB::Red, CRGB::Blue );
        }
        else{
            //nothing interesting yet
            setMeterValues( meters->center, 0, 100, CRGB::Black, CRGB::Black );
        }
    }  
}
void flashMetersIfTimeIsExpired(GameState* game, GameSettings* settings, MeterSettings* meters){
    if ( game->time.timeExpired){
        setMetersToFlashInterval(meters, FlashInterval::FLASH_FAST);
    }
    else{
        setMetersToFlashInterval(meters, FlashInterval::FLASH_NONE);
    }
}

void setAllMetersToSolidColor(MeterSettings* meters, CRGB fg, CRGB bg){
    setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg );
    setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg );
    setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg );
    setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg );  
    setMeterValues( meters->center, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg ); 
    setMeterValues( meters->left, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg );
    setMeterValues( meters->right, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, fg, bg );   
}

void updateMeters(GameState* game, GameSettings* settings, MeterSettings* meters){
    flashMetersIfTimeIsExpired(game,settings,meters);

    if ( game->status == GAME_STATUS_PREGAME){
        setAllMetersToSolidColor(meters,CRGB::Yellow, CRGB::Black);
    }
    else {
        updateMetersForRunningGame(game,settings,meters);
    }
}

//TODO: tons of duplication here. 
//is there an easier way? 
void setupMeters(GameState* gs, GameSettings* settings, MeterSettings* meters){
    if ( settings->gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        configureMeter(meters->center, STANDARD_METER_MAX_VAL, 0, CRGB::Black, CRGB::Black);   //NOT USED in this mode
        configureMeter(meters->left, settings->hits.to_win, 0, CRGB::Blue, CRGB::Black); //hits scored for blue , count from zero to total required to win
        configureMeter(meters->right, settings->hits.to_win, 0, CRGB::Red, CRGB::Black); //hits scored for red , count from zero to total required to win
    }
    else if ( settings->gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        configureMeter(meters->center, STANDARD_METER_MAX_VAL, 0, CRGB::Red, CRGB::Blue);                          // ratio of red to total hits.  max_val gets changed once there are non-zero total hits
        configureMeter(meters->left, settings->timed.max_duration_seconds, 0, CRGB::Blue, CRGB::Black);      // game progress, count from zero to total game duration
        configureMeter(meters->right, settings->timed.max_duration_seconds, 0, CRGB::Red, CRGB::Black);      // game progress, count from zero to total game duration
    }
    else if ( settings->gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME ){
        configureMeter(meters->center, settings->capture.hits_to_capture, 0, CRGB::Red, CRGB::Blue);       // capture progress: background = owner, foreground: count up from zero to hits required to capture
        configureMeter(meters->left, settings->timed.ownership_time_seconds, 0, CRGB::Blue, CRGB::Black);  //ownership time for blue, count from zero to time required to win
        configureMeter(meters->right, settings->timed.ownership_time_seconds, 0, CRGB::Red, CRGB::Black);   //ownership time for red, count from zero to time required to win
    }
    else if ( settings->gameType == GameType::GAME_TYPE_ATTACK_DEFEND ){
        configureMeter(meters->center, settings->capture.hits_to_capture, 0, CRGB::Blue, CRGB::Black);   //hit progress: count from zero to hits required to win
        configureMeter(meters->left, settings->capture.hits_to_capture, 0, CRGB::Blue, CRGB::Black);    //hit progress: count from zero to hits required to win
        configureMeter(meters->right, settings->capture.hits_to_capture, 0, CRGB::Blue, CRGB::Black);    //hit progress: count from zero to hits required to win
    }
    else if ( settings->gameType == GameType::GAME_TYPE_TARGET_TEST ){
        configureMeter(meters->center, settings->hits.to_win, 0, CRGB::Red, CRGB::Blue);  
        configureMeter(meters->left, settings->hits.to_win, 0, CRGB::Blue, CRGB::Black); //hits scored for blue , count from zero to total required to win
        configureMeter(meters->right, settings->hits.to_win, 0, CRGB::Red, CRGB::Black); //hits scored for red , count from zero to total required to win
    }    
    else{
       Log.errorln("UNKNOWN GAME TYPE");
    }
}

void startGame(GameState* gs, GameSettings* settings, Clock* clock){
    long current_time_millis = clock->milliseconds();
    gs->time.start_time_millis = current_time_millis;
    gs->bluHits.hits = 0;
    gs->bluHits.last_decay_millis = current_time_millis;
    gs->bluHits.last_hit_millis = 0;
    gs->redHits.hits = 0;
    gs->redHits.last_decay_millis = current_time_millis;
    gs->redHits.last_hit_millis = 0;    
    gs->status = GameStatus::GAME_STATUS_PREGAME;
    gs->result.winner = Team::NOBODY;
    gs->ownership.blu_millis = 0;
    gs->ownership.red_millis = 0;
    gs->ownership.owner = Team::NOBODY;
    gs->ownership.capturing = Team::NOBODY;
    gs->ownership.capture_hits = 0;

    if ( settings->gameType == GAME_TYPE_ATTACK_DEFEND){
        gs->ownership.capturing = Team::BLU;  
    }

}

//TODO: duplicated with below. combine this, its nearly identical between red and blue
void applyLeftHits(GameState* current, GameSettings* settings,TargetHitData hitdata, long current_time_millis){
    if ( hitdata.hits > 0 ){
        //used mainly in hit-count games
        current->redHits.hits += hitdata.hits;
        current->redHits.last_hit_energy = hitdata.last_hit_energy;
        current->redHits.last_hit_millis = current_time_millis;
        current->redHits.last_decay_millis = current_time_millis;
    }
    if ( settings->gameType == GAME_TYPE_KOTH_FIRST_TO_OWN_TIME || settings->gameType == GAME_TYPE_KOTH_MOST_OWN_IN_TIME ){
        if ( current->ownership.capturing == Team::RED){
           current->ownership.capture_hits += hitdata.hits;
        }
    }
    else if (settings->gameType == GAME_TYPE_ATTACK_DEFEND ){
         current->ownership.capture_hits += hitdata.hits;
    }
}

void applyRightHits(GameState* current, GameSettings* settings,TargetHitData hitdata, long current_time_millis){
    if ( hitdata.hits > 0 ){
        //used mainly in hit-count games
        current->bluHits.hits += hitdata.hits;
        current->bluHits.last_hit_energy = hitdata.last_hit_energy;
        current->bluHits.last_hit_millis = current_time_millis;
        current->bluHits.last_decay_millis = current_time_millis;
    }
    if ( settings->gameType == GAME_TYPE_KOTH_FIRST_TO_OWN_TIME || settings->gameType == GAME_TYPE_KOTH_MOST_OWN_IN_TIME ){
        if ( current->ownership.capturing == Team::BLU){
            current->ownership.capture_hits += hitdata.hits;
        }
    }  
    else if (settings->gameType == GAME_TYPE_ATTACK_DEFEND ){
         current->ownership.capture_hits += hitdata.hits;
    }      
}

//assumes that captureHits is being updated first
//TODO: extract core decay function duplicated with below
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

//TODO: factor out decay logic from game logic
void applyTestModeHitDecay(GameState* current, GameSettings settings, long current_time_millis){

    long decay_millis = settings.capture.capture_decay_rate_secs_per_hit*1000;

    long millis_since_last_red_hit =  ( current_time_millis - current->redHits.last_hit_millis);
    long millis_since_last_red_decay =  ( current_time_millis - current->redHits.last_decay_millis);

    long millis_since_last_blu_hit =  ( current_time_millis - current->bluHits.last_hit_millis);
    long millis_since_last_blu_decay =  ( current_time_millis - current->bluHits.last_decay_millis);
    

    //TODO: remove duplication below
    if ( (millis_since_last_red_hit > decay_millis) && (millis_since_last_red_decay >  decay_millis)){
        if ( current->redHits.hits > 0 ){
            Log.warningln("Decaying Red: since_decay=%l, since_hit=%l", millis_since_last_red_decay,millis_since_last_red_hit);
            current->redHits.hits--;
            current->redHits.last_decay_millis = current_time_millis;
        }
    }
    if ( (millis_since_last_blu_hit > decay_millis) && (millis_since_last_blu_decay >  decay_millis)){
        if ( current->bluHits.hits > 0 ){
            Log.warningln("Decaying Blue: since_decay=%l, since_hit=%l", millis_since_last_blu_decay,millis_since_last_blu_hit);
            current->bluHits.hits--;
            current->bluHits.last_decay_millis = current_time_millis;
        }
    }
}


//updates timeExpired based on game state. 
//makes testing easier, so we dont have to set up all those conditions
void updateGameTime(GameState* current,GameSettings settings, long current_time_millis){

    long second_elapsed_since_start = (current_time_millis - current->time.start_time_millis)/1000;
    current->time.elapsed_secs = second_elapsed_since_start - settings.timed.countdown_start_seconds; //can be negative

    bool isExpired = false;
    bool isOvertimeExpired = false;

    if ( current->time.elapsed_secs >= 0){
        current->status = GAME_STATUS_RUNNING;
        if ( settings.timed.max_duration_seconds > 0){
            if ( current->time.elapsed_secs > settings.timed.max_duration_seconds){
                isExpired =  true;
            }
            if ( current->time.elapsed_secs > (settings.timed.max_duration_seconds + settings.timed.max_overtime_seconds)){
                isOvertimeExpired = true;
            }
        }
    }
    else{
        current->status = GAME_STATUS_PREGAME;
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

void endGameWithMostOwnership(GameState* current){
    if ( current->ownership.blu_millis > current->ownership.red_millis){
        endGame(current, Team::BLU);
    }
    else if (current->ownership.red_millis > current->ownership.blu_millis ){
        endGame(current, Team::RED);
    }
    else{
        endGameWithMostHits(current,current->redHits.hits,current->bluHits.hits);
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

        METERS          val                 max-val                     fgColor     bgColor
        --------------------------------------------------------------------------------------
        left            red_hits            hits to win                 red         black   
        left-top        10,flashing in OT   10                          red         black
        left-bottom     10,flashing in OT   10                          red         black
        center          0(OFF)              10                          black       black
        right-top       10,flashing in OT   10                          blue        black
        right           blu_hits            hits to win                 blue        black
        right-bottom    10,flashing in OT   10                          blue        black
        
    */
    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;
    Log.traceln("RedHits=%d, BluHits=%d",red_hits,blu_hits);

    Team winner = getWinnerByHitsWithVictoryMargin(red_hits, blu_hits, settings.hits.to_win, settings.hits.victory_margin );
    Team closeToWinner = getWinnerByHitsWithoutVictoryMargin(red_hits,blu_hits,settings.hits.to_win);

    if ( winner != Team::NOBODY){
        endGame(current, winner);
    }
    else if ( current->time.overtimeExpired  ){ 
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( closeToWinner != Team::NOBODY){
        current->status = GameStatus::GAME_STATUS_OVERTIME;
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

        METERS          val                 max-val                     fgColor     bgColor
        ----------------------------------------------------------------------------------------
        left            red_hits            red_hits + blu_hits         red         black   
        left-top        10,flashing in OT   10                          red         black
        left-bottom     10,flashing in OT   10                          red         black
        center          red_hits            red_hits + blu_hits         red         blue           ( black/black when no hits yet)
        right-top       10,flashing in OT   10                          blue        black
        right           blu_hits            red_hits + blu_hits         blue        black
        right-bottom    10,flashing in OT   10                          blue        black

        
    */
    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;
    

    Team winner = getWinnerByHitsWithVictoryMargin(red_hits, blu_hits, settings.hits.to_win, settings.hits.victory_margin );
    Team closeToWinner = getWinnerByHitsWithoutVictoryMargin(red_hits,blu_hits,settings.hits.to_win);

    if ( current->time.overtimeExpired ){
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( current->time.timeExpired ){
        if ( winner != Team::NOBODY){
            endGame(current, winner);
        }
        else if ( closeToWinner != Team::NOBODY){
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }        
        else{
            current->status = GameStatus::GAME_STATUS_OVERTIME;    
        }
    }
    else{
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }

}

void updateAttackDefendGame(GameState* current,  GameSettings settings, long current_time_millis){
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

        METERS          val                 max-val             fgColor     bgColor
        ----------------------------------------------------------------------------
        left            blue_hits+red_hits  hits_to_capture     blue         black
        left-top        10,flashing in OT   10                  blue         black
        left-bottom     10,flashing in OT   10                  blue         black
        center          blue_hits+red_hits  hits_to_capture     blue         black
        right-top       10,flashing in OT   10                  blue         black
        right           blue_hits+red_hits  hits_to_capture     blue         black
        right-bottom    10,flashing in OT   10                  blue         black

    */

    applyHitDecay(current, settings,current_time_millis);

    int total_hits = current->ownership.capture_hits;
    int hits_to_capture = settings.capture.hits_to_capture;

    if ( total_hits >= hits_to_capture){
        endGame(current, Team::BLU);
    }
    else if ( current->time.overtimeExpired){
        endGame(current, Team::RED);
    }
    else if ( current->time.timeExpired ){
        if ( total_hits > (hits_to_capture - settings.hits.victory_margin) ){
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            endGame(current, Team::RED);
        }
    }
    else {
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }   
}
void triggerOvertime(GameState* current,  GameSettings settings){
    Log.infoln("Overtime Triggered!");
    current->ownership.overtime_remaining_millis = settings.capture.capture_overtime_seconds;
}

void capture(GameState* current,  Team capturingTeam){
    current->ownership.owner = capturingTeam;
    Log.infoln("Control Point has been captured by: %c", teamTextChar(capturingTeam));
    current->ownership.capturing = oppositeTeam(capturingTeam);      
    current->ownership.just_captured = capturingTeam;
    current->ownership.capture_hits = 0;
}

void updateOwnership(GameState* current,  GameSettings settings, long current_time_millis){

    long elapsed_since_last_update = (current_time_millis - current->time.last_update_millis);


    if ( current->ownership.owner== Team::NOBODY){
        //waiting for the first capture!
        //before capture, we use the regular hit counters, because either team can capture
        //if somehow both teams get enough hits in the same update interval( unlikely, the bigger wins it)

        //TODO: this logic is materially duplicated in the updateMeter area. need to put 
        //compute here and give a simple value in game state the meter can use
        //if only one team has capture hits, show progress towards capture.
        //if both teams have capture hits, one team must get more than the other by the capture_hits margin.
        int blu_hits = current->bluHits.hits;
        int red_hits = current->redHits.hits;
        int hit_diff = abs(blu_hits - red_hits);
        if ( hit_diff >= settings.capture.hits_to_capture){
            if ( blu_hits > red_hits){
                capture(current, Team::BLU);
            }
            else{
                capture(current, Team::RED);
            }
        }
    }
    else{
        if ( current->ownership.owner== Team::RED){
            current->ownership.red_millis += elapsed_since_last_update;
        }
        else if ( current->ownership.owner== Team::BLU){
            current->ownership.blu_millis += elapsed_since_last_update;
        }
        Log.infoln("Checking for Capture, %d/%d to capture.",current->ownership.capture_hits,settings.capture.hits_to_capture);
        if ( current->ownership.capture_hits >= settings.capture.hits_to_capture ){
            triggerOvertime(current,settings);
            capture(current,current->ownership.capturing);
        }
    }

}

void updateFirstToOwnTimeGame(GameState* current,  GameSettings settings, long current_time_millis){
    /*
        VICTORY
        Capturing the point requires a given number of hits.
        Once the point is captured, the owning team accumulates time. 

        The winner is the first team to accumulate the given amount of ownership time, 
        AND survive any contests.

        HITS
        in this game, hits decay at the rate provided by capture settings. 

        at the beginning of the game, when nobody has captured the point, both teams can capture.
        If one team achives more hits than the other by the capture margin, that team caputures. 
        It's essentially tug of war till the capture margin is reached.
        Example: 5 hits required to capture: if red has no hits, blue can capture with 5. but if red has 4, blue will need 9 to capture  

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
            if that's tied, the team with most hits wins
            if hit counts are tied too, the game ends in a TIE

        METERS          val                 max-val             fgColor     bgColor
        ----------------------------------------------------------------------------
        left            red_own_time        own_time_to_win     red         black
        left-top        10,flashing in OT   10                  red         black
        left-bottom     10,flashing in OT   10                  red         black
        center          capture_hits        hits_to_capture     capturing   owner(yellow if no owner)
        right-top       10,flashing in OT   10                  blue         black
        right           blu_own_time        own_time_to_win     blue         black
        right-bottom    10,flashing in OT   10                  blue         black


    */     

    applyHitDecay(current, settings,current_time_millis);
    updateOwnership(current,settings,current_time_millis);

    long ownership_time_to_win_millis = 1000*settings.timed.ownership_time_seconds;
    bool blue_time_complete = (current->ownership.blu_millis > ownership_time_to_win_millis);
    bool red_time_complete = (current->ownership.red_millis > ownership_time_to_win_millis);

    bool isContested  = ( current->ownership.overtime_remaining_millis > 0 );
    bool isTimeExpired = current->time.timeExpired;
    bool isOverTimeExpired = current->time.overtimeExpired;

    if ( isOverTimeExpired ){
        Log.infoln("Max Overtime Expired. Forcing Game End");
        endGameWithMostOwnership(current);
    }
    else if ( isTimeExpired ){
        if ( isContested){
            Log.infoln("Time is expired, but point is still contested.");
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            Log.infoln("Time is expired, ending with most ownership");
            endGameWithMostOwnership(current);
        }
    }
    else if ( blue_time_complete ){
        if ( isContested ){
            Log.infoln("Blue is about to win, but we are in overtime");
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            Log.infoln("Blue wins");
            endGame(current,Team::BLU);
        }
    }
    else if ( red_time_complete ){
        if ( isContested ){
            Log.infoln("Red is about to win, but we are in overtime");
            current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            Log.infoln("Red wins");
            endGame(current,Team::RED);
        }
    }
    else {
        Log.infoln("nothing interesting going on.");
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }   

     
}

void updateTargetTestMode(GameState* current,  GameSettings settings, long current_time_millis){
    /* 
      This is a special game mode that's for tuning the targets.
      It never ends unless ended manually.

      It counts hits constantly, resetting the meter when necessary
      */

    applyTestModeHitDecay(current, settings,current_time_millis);

    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;
    int diff = abs(red_hits - blu_hits);

    Log.traceln("RedHits=%d, BluHits=%d",red_hits,blu_hits);

    if ( diff > settings.hits.victory_margin ){
        endGameWithMostHits(current,red_hits,blu_hits);
    }
    else{
        //never ends except manually
        current->status = GameStatus::GAME_STATUS_RUNNING;    
    }

}
void updateMostOwnInTimeGame(GameState* current,  GameSettings settings, long current_time_millis){
    endGame(current, Team::NOBODY);
}

void updateGame(GameState* current,  GameSettings settings, Clock* clock){

    updateGameTime(current, settings, clock->milliseconds());

    if ( current->status != GAME_STATUS_PREGAME){
        Log.traceln("After Update, Hits:B=%d,R=%d",current->bluHits.hits, current->redHits.hits);
        if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
            updateFirstToHitsGame(current,settings);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME){
            updateMostHitsInTimeGame(current,settings);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME){
            updateFirstToOwnTimeGame(current,settings,clock->milliseconds());
        }
        else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND){
            updateAttackDefendGame(current,settings,clock->milliseconds());
        }
        else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_OWN_IN_TIME){
            updateMostOwnInTimeGame(current,settings,clock->milliseconds());
        }
        else if ( settings.gameType == GameType::GAME_TYPE_TARGET_TEST){
            updateTargetTestMode(current,settings,clock->milliseconds());
        }
        else{
            Log.errorln("Unknown Game Type: %d", settings.gameType);
        }
    }
    //very last to make sure everyone can see the time delta during update
    current->time.last_update_millis = clock->milliseconds();
}


