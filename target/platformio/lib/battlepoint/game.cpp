#include <game.h>
#include <Teams.h>
#include <math.h>
#include <target.h>

//TODO: dont want game to know about meters at all.
#include <LedMeter.h>
#include <ArduinoLog.h>

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

void default_handle_game_statuschange(GameStatus oldstatus, GameStatus newstatus, Team winner){};
void default_handle_game_contested(){}
void default_handle_game_capture(Team capturing){}
void default_handle_game_remainingsecs(int secs_remaining, GameStatus status){}

void supplyDefaultHandlers(GameState* state){
    if ( state->eventHandler.CapturedHandler == 0 ){
        state->eventHandler.CapturedHandler = default_handle_game_capture;
    }
    if ( state->eventHandler.ContestedHandler == 0 ){
        state->eventHandler.ContestedHandler = default_handle_game_contested;
    }
    if ( state->eventHandler.StatusChangeHandler == 0 ){
        state->eventHandler.StatusChangeHandler = default_handle_game_statuschange;
    }
    if ( state->eventHandler.RemainingSecsHandler == 0 ){
        state->eventHandler.RemainingSecsHandler = default_handle_game_remainingsecs;
    }
}

void resetGame(GameState* state){
    state->status = GameStatus::GAME_STATUS_NOTSTARTED;

    state->time.elapsed_secs=0;
    state->time.last_update_millis=0;
    state->time.start_time_millis = 0;
    state->time.remaining_secs=0;
    state->time.timeExpired=false;
    state->time.overtimeExpired=false;
    
    state->result.winner = Team::NOBODY;

    state->bluHits.hits = 0;
    state->bluHits.last_decay_millis = 0;
    state->bluHits.last_hit_millis = 0;
    state->bluHits.last_hit_energy = 0;
    state->redHits.hits = 0;
    state->redHits.last_decay_millis = 0;
    state->redHits.last_hit_millis = 0;    
    state->redHits.last_hit_energy = 0;
    
    state->ownership.blu_millis = 0;
    state->ownership.red_millis = 0;
    state->ownership.owner = Team::NOBODY;
    state->ownership.capturing = Team::NOBODY;
    state->ownership.capture_hits = 0;
    state->ownership.overtime_remaining_millis=0;
    state->ownership.last_decay_millis=0;
    state->ownership.last_hit_millis=0; 
}

void changeGameStatus(GameState* state, GameStatus newStatus ){
    
    if ( state->status != newStatus){
        GameStatus oldStatus = state->status;
        Log.infoln("New Status: %c",newStatus);
        Team t = Team::NOBODY;
        if ( newStatus == GameStatus::GAME_STATUS_ENDED){
            t = state->result.winner;
        }
        state->status = newStatus;
        state->eventHandler.StatusChangeHandler(oldStatus,newStatus,t);        
    }
}

void startGame(GameState* gs, GameSettings* settings, long current_time_millis){
    resetGame(gs);
    supplyDefaultHandlers(gs);
    gs->time.start_time_millis = current_time_millis;
    gs->bluHits.last_decay_millis = current_time_millis;
    gs->redHits.last_decay_millis = current_time_millis;
    changeGameStatus(gs,GameStatus::GAME_STATUS_PREGAME);
    if ( settings->gameType == GAME_TYPE_ATTACK_DEFEND){
        gs->ownership.capturing = Team::BLU;  
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

//TODO: this needs more de-coupling, with callbacks
//so that other types of feecback/meters can be implmeented
//but thats best done when we know the other kind of meters we have

void updateMetersForRunningGame(GameState* game, GameSettings* settings, MeterSettings* meters){
    setHorizontalMetersToTeamColors(meters);
    if ( settings->gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        setMeterValues( meters->center, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Black, CRGB::Black ); 
        setMeterValues( meters->left, game->redHits.hits, settings->hits.to_win, CRGB::Red, CRGB::Black );
        setMeterValues( meters->right, game->bluHits.hits, settings->hits.to_win, CRGB::Blue, CRGB::Black );
    }
    else if ( settings->gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        int total_hits = game->bluHits.hits + game->redHits.hits;
    
        if ( total_hits  > 0 ){
            setMeterValues(meters->center, game->redHits.hits, total_hits, CRGB::Red, CRGB::Blue);
            setMeterValues(meters->left, game->redHits.hits, total_hits, CRGB::Red, CRGB::Black);
            setMeterValues(meters->right, game->bluHits.hits, total_hits, CRGB::Blue, CRGB::Black);    
        }
        else{
            setMeterValues(meters->left, 0, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black);
            setMeterValues(meters->right, 0, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black);                
            setMeterValues(meters->center, 0, STANDARD_METER_MAX_VAL, CRGB::Black, CRGB::Black);                 
        }

    }
    else if ( settings->gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME ){
        //update meters
        int secs_to_win = settings->timed.ownership_time_seconds;
        int red_own_secs = game->ownership.red_millis/1000;
        int blue_own_secs = game->ownership.blu_millis/1000;


        //normally the team meters are progress towards seconds to win. if, however, we are
        //in overtime, and one team has more time than the other, we udpate max value 
        //to the larger value, so that its easy to see who the winner is
        int ownership_meter_max_val = max(max(red_own_secs, blue_own_secs),secs_to_win);
        
        setMeterValues( meters->left, red_own_secs, ownership_meter_max_val, CRGB::Red, CRGB::Black );
        setMeterValues( meters->right, blue_own_secs, ownership_meter_max_val, CRGB::Blue, CRGB::Black ); 

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

void updateMetersDuringPregame(GameState* game, GameSettings* settings, MeterSettings* meters){
    setHorizontalMetersToNeutralColors(meters);

    setMeterValues( meters->center, -game->time.remaining_secs, settings->timed.countdown_start_seconds, CRGB::Yellow, CRGB::Black ); 
    setMeterValues( meters->left, -game->time.remaining_secs, settings->timed.countdown_start_seconds, CRGB::Yellow, CRGB::Black );
    setMeterValues( meters->right, -game->time.remaining_secs, settings->timed.countdown_start_seconds, CRGB::Yellow, CRGB::Black ); 
}

void updateMeters(GameState* game, GameSettings* settings, MeterSettings* meters){
    flashMetersIfTimeIsExpired(game,settings,meters);

    if ( game->status == GAME_STATUS_PREGAME){
        updateMetersDuringPregame(game,settings,meters);
    }
    else {
        updateMetersForRunningGame(game,settings,meters);
    }
}

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

void applyHitsTo (GameType gameType, TargetHitData hitdata, HitTracker* hits, Ownership* ownership, Team capturing, Team  hitting, long current_time_millis){
    if ( hitdata.hits > 0 ){
        //used mainly in hit-count games
        hits->hits += hitdata.hits;
        hits->last_hit_energy = hitdata.last_hit_energy;
        hits->last_hit_millis = current_time_millis;
        hits->last_decay_millis = current_time_millis;
    }
    if ( gameType == GAME_TYPE_KOTH_FIRST_TO_OWN_TIME || gameType == GAME_TYPE_KOTH_MOST_OWN_IN_TIME ){
        if ( capturing == hitting){
           ownership->capture_hits += hitdata.hits;
        }
    }
    else if (gameType == GAME_TYPE_ATTACK_DEFEND ){
           ownership->capture_hits += hitdata.hits;
    }
} 

bool canApplyHits(GameState* current){
    return ( current->status == GameStatus::GAME_STATUS_OVERTIME || current->status == GameStatus::GAME_STATUS_RUNNING);
}

void applyLeftHits(GameState* current, GameSettings* settings,TargetHitData hitdata, long current_time_millis){
    if ( canApplyHits(current)){
        applyHitsTo (settings->gameType, hitdata, &current->redHits, &current->ownership, current->ownership.capturing, Team::RED, current_time_millis);
    }
    
}

void applyRightHits(GameState* current, GameSettings* settings,TargetHitData hitdata, long current_time_millis){
    if ( canApplyHits(current)){
        applyHitsTo (settings->gameType, hitdata, &current->bluHits, &current->ownership, current->ownership.capturing, Team::BLU, current_time_millis); 
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

void applyTestModeHitDecayToHitTracker( HitTracker* tracker, long current_time_millis, long decay_millis){
    long millis_since_last_hit =  ( current_time_millis - tracker->last_hit_millis);
    long millis_since_last_decay =  ( current_time_millis - tracker->last_decay_millis);

    if ( (millis_since_last_hit > decay_millis) && (millis_since_last_decay >  decay_millis)){
        if ( tracker->hits > 0 ){
            Log.warningln("Decaying Red: since_decay=%l, since_hit=%l", millis_since_last_decay,millis_since_last_hit);
            tracker->hits--;
            tracker->last_decay_millis = current_time_millis;
        }
    }
}

//TODO: factor out decay logic from game logic
void applyTestModeHitDecay(GameState* current, GameSettings settings, long current_time_millis){

    long decay_millis = settings.capture.capture_decay_rate_secs_per_hit*1000;
    applyTestModeHitDecayToHitTracker( &current->redHits, current_time_millis, decay_millis);
    applyTestModeHitDecayToHitTracker( &current->bluHits, current_time_millis, decay_millis);
}


//important note: 
//this function is not reesponsible for deciding when overtime is triggered,
//because more game rules apply there that just the time elapsed
//see triggerOvertime

void updateGameTime(GameState* current,GameSettings settings, long current_time_millis){

    int second_elapsed_since_start = (current_time_millis - current->time.start_time_millis)/1000;    
    int game_seconds_elapsed = second_elapsed_since_start - settings.timed.countdown_start_seconds;
    current->time.elapsed_secs = game_seconds_elapsed;
    bool isExpired = false;
    bool isOvertimeExpired = false;

    if ( current->status == GameStatus::GAME_STATUS_PREGAME){
        int seconds_till_game_start = settings.timed.countdown_start_seconds - second_elapsed_since_start;
        
        //in the context of pregame, remaining seconds is time till the start, but negative  
        current->time.remaining_secs = game_seconds_elapsed;

        if (seconds_till_game_start <= 0  ){
            changeGameStatus(current, GameStatus::GAME_STATUS_RUNNING);
        }
    }
    else if ( current->status == GameStatus::GAME_STATUS_RUNNING){

        //NOTE : this code doesnt trigger overtime, beacuse it doesnt always happen based only on time. 
        int seconds_till_game_end = settings.timed.max_duration_seconds - game_seconds_elapsed;
        
        //in the context of running game, remaining seconds is time till end of regulation ( not including overtime)
        current->time.remaining_secs =seconds_till_game_end;
        if ( seconds_till_game_end <= 0 ){
            isExpired = true;
        }        
    }
    else if ( current->status == GameStatus::GAME_STATUS_OVERTIME){
        int seconds_till_overtime_end = settings.timed.max_duration_seconds + settings.timed.max_overtime_seconds - game_seconds_elapsed ;

        //in the context of overtime, remaining seconds is time till end of overtime 
        current->time.remaining_secs =seconds_till_overtime_end;
        if ( seconds_till_overtime_end <= 0){
            isOvertimeExpired = true;
        }
    }
    else if ( current->status == GameStatus::GAME_STATUS_ENDED){
        current->time.remaining_secs = 0;
        isExpired = true;
        isOvertimeExpired = true;
    }
    Log.infoln("Time(s): Since Start: %d, Remaining: %d", current->time.elapsed_secs,current->time.remaining_secs);
    current->eventHandler.RemainingSecsHandler(current->time.remaining_secs,current->status);
    current->time.timeExpired = isExpired;
    current->time.overtimeExpired = isOvertimeExpired;
}

void endGame( GameState* current, Team winner){    
    current->result.winner = winner;        
    Log.infoln("Ending Game, winner=%d",current->result.winner);
    changeGameStatus(current, GameStatus::GAME_STATUS_ENDED);
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

void triggerOvertime(GameState* current,  GameSettings settings){
    Serial.print("in triggerOvertime, status = ");Serial.println(current->status);
    if ( GameStatus::GAME_STATUS_OVERTIME != current->status ){
        changeGameStatus(current, GameStatus::GAME_STATUS_OVERTIME);
        Log.infoln("Overtime Triggered!");
        current->ownership.overtime_remaining_millis = settings.capture.capture_overtime_seconds;
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
    Log.infoln("RedHits=%d, BluHits=%d",red_hits,blu_hits);

    Team winner = getWinnerByHitsWithVictoryMargin(red_hits, blu_hits, settings.hits.to_win, settings.hits.victory_margin );
    Team closeToWinner = getWinnerByHitsWithoutVictoryMargin(red_hits,blu_hits,settings.hits.to_win);

    if ( winner != Team::NOBODY){
        endGame(current, winner);
    }
    else if ( current->time.overtimeExpired  ){ 
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( closeToWinner != Team::NOBODY){
        triggerOvertime(current,settings);
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
    //Team closeToWinner = getWinnerByHitsWithoutVictoryMargin(red_hits,blu_hits,settings.hits.to_win);

    if ( current->time.overtimeExpired ){
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( current->time.timeExpired ){
        if ( winner != Team::NOBODY){
            endGame(current, winner);
        }     
        else{
            triggerOvertime(current,settings);    
        }
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
    Log.infoln("AD: %d/%d hits", total_hits, hits_to_capture);
    if ( total_hits >= hits_to_capture){
        Log.infoln("AD: %d/%d hits, game over!", total_hits, hits_to_capture);
        endGame(current, Team::BLU);
    }
    else if ( current->time.overtimeExpired){
        endGame(current, Team::RED);
    }
    else if ( current->time.timeExpired ){
        if ( total_hits >= (hits_to_capture - settings.hits.victory_margin) ){
            triggerOvertime(current,settings);
        }
        else{
            endGame(current, Team::RED);
        }
    } 
}

void capture(GameState* current,  Team capturingTeam){
    current->ownership.owner = capturingTeam;
    Log.infoln("Control Point has been captured by: %c", teamTextChar(capturingTeam));
    current->ownership.capturing = oppositeTeam(capturingTeam);      
    current->ownership.capture_hits = 0;
    current->ownership.notifiedContested = false;
    current->eventHandler.CapturedHandler(capturingTeam);
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
            capture(current,current->ownership.capturing);
        }
    }

}

bool isControlPointContested( Ownership* ownership){
    return isHumanTeam(ownership->owner) && ownership->capture_hits > 0;
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

    //bool isContested  = ( current->ownership.overtime_remaining_millis > 0 );
    bool isTimeExpired = current->time.timeExpired;
    bool isOverTimeExpired = current->time.overtimeExpired;
    bool isContested = isControlPointContested(&current->ownership);

    if ( ! current->ownership.notifiedContested && isContested ){
            current->eventHandler.ContestedHandler();
            current->ownership.notifiedContested = true;
    }

    if ( current->status == GameStatus::GAME_STATUS_RUNNING){
        if ( blue_time_complete && red_time_complete){
            Log.infoln("Both teams have enough time to win. Overtime!");
            triggerOvertime(current,settings);            
        }
        else if ( blue_time_complete ){
            if ( current->ownership.capturing == Team::RED && isContested){
                triggerOvertime(current,settings);            
            }
            else{
                Log.infoln("Blue wins");
                endGame(current,Team::BLU);                
            }
        }
        else if ( red_time_complete ){
            if ( current->ownership.capturing == Team::BLU && isContested){
                triggerOvertime(current,settings);            
            }
            else{
                Log.infoln("Red wins");
                endGame(current,Team::RED);                
            }
        }
        else if ( isTimeExpired ){
            if ( isContested){
                triggerOvertime(current,settings);            
            }
            else{
                endGameWithMostOwnership(current);
            }
            
        }
    }
    else if ( current->status ==  GameStatus::GAME_STATUS_OVERTIME){
        if ( isOverTimeExpired ){
            Log.infoln("Max Overtime Expired. Forcing Game End");
            endGameWithMostOwnership(current);
        }    
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
}
void updateMostOwnInTimeGame(GameState* current,  GameSettings settings, long current_time_millis){
    endGame(current, Team::NOBODY);
}

void updateGame(GameState* current,  GameSettings settings, long current_time_millis){

    updateGameTime(current, settings, current_time_millis);
    if ( current->status == GAME_STATUS_RUNNING || current->status == GAME_STATUS_OVERTIME){            
        Log.traceln("After Update, Hits:B=%d,R=%d",current->bluHits.hits, current->redHits.hits);
        if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
            updateFirstToHitsGame(current,settings);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME){
            updateMostHitsInTimeGame(current,settings);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME){
            updateFirstToOwnTimeGame(current,settings,current_time_millis);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND){
            updateAttackDefendGame(current,settings,current_time_millis);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_OWN_IN_TIME){
            updateMostOwnInTimeGame(current,settings,current_time_millis);
        }
        else if ( settings.gameType == GameType::GAME_TYPE_TARGET_TEST){
            updateTargetTestMode(current,settings,current_time_millis);
        }
        else{
            Log.errorln("Unknown Game Type: %d", settings.gameType);
        }
    }
    //very last to make sure everyone can see the time delta during update
    current->time.last_update_millis = current_time_millis;
    Log.infoln("Current Status: %c",current->status);
}


