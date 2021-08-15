#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>
#include <target.h>
#include <LedMeter.h>
#include <ArduinoLog.h>



GameSettings DEFAULT_GAMESETTINGS(){
    GameSettings gs;
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

    gs.target.hit_energy_threshold = 5000;
    gs.target.last_hit_millis=0;
    gs.target.trigger_threshold=100;
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


void updateLeds(GameState* current, long current_time_millis ){
  MeterSettings ms = current->meters;
  updateController( &ms.left, current_time_millis);
  updateController( &ms.center, current_time_millis);
  updateController( &ms.right, current_time_millis);
  updateController( &ms.leftTop, current_time_millis);
  updateController( &ms.rightTop, current_time_millis);
  updateController( &ms.leftBottom, current_time_millis);
  updateController( &ms.rightBottom , current_time_millis);    
}

GameState startGame(GameSettings settings, Clock* clock, MeterSettings base_meters){
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
    gs.meters = base_meters;

    if ( settings.gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
        configureMeter(&gs.meters.center.meter, STANDARD_METER_MAX_VAL, 0, CRGB::Black, CRGB::Black);   //NOT USED in this mode
        configureMeter(&gs.meters.left.meter, settings.hits.to_win, 0, CRGB::Blue, CRGB::Black); //hits scored for blue , count from zero to total required to win
        configureMeter(&gs.meters.right.meter, settings.hits.to_win, 0, CRGB::Red, CRGB::Black); //hits scored for red , count from zero to total required to win
    }
    else if ( settings.gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        configureMeter(&gs.meters.center.meter, STANDARD_METER_MAX_VAL, 0, CRGB::Red, CRGB::Blue);                          // ratio of red to total hits.  max_val gets changed once there are non-zero total hits
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
    else if ( settings.gameType == GameType::GAME_TYPE_TARGET_TEST ){
        configureMeter(&gs.meters.center.meter, STANDARD_METER_MAX_VAL, 0, CRGB::Black, CRGB::Black);   //NOT USED in this mode
        configureMeter(&gs.meters.left.meter, settings.hits.to_win, 0, CRGB::Blue, CRGB::Black); //hits scored for blue , count from zero to total required to win
        configureMeter(&gs.meters.right.meter, settings.hits.to_win, 0, CRGB::Red, CRGB::Black); //hits scored for red , count from zero to total required to win
    }    
    else{
       Log.errorln("UNKNOWN GAME TYPE");
    }
    return gs;

}



void updateGameHitsSingleSensor(HitTracker* tracker, TargetHitScanResult scanResult, long current_time_millis){
    if ( scanResult.was_sampled){
        if ( scanResult.was_hit ){
            tracker->hits++;
            tracker->last_hit_millis = current_time_millis;    
            tracker->last_hit_energy = scanResult.last_hit_energy;
        } 
    }    
}
void updateGameHits(GameState* current, SensorState sensors, long current_time_millis){
    updateGameHitsSingleSensor(&current->bluHits,sensors.rightScan,current_time_millis);
    updateGameHitsSingleSensor(&current->redHits,sensors.leftScan,current_time_millis);
}
//assumes that captureHits is being updated first
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
    current->time.elapsed_secs = elapsed_since_start / 1000;
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

void setFlashMeterForTeam(Team t, GameState* current, FlashInterval fi ){
    if ( t == Team::RED ){
        current->meters.leftBottom.meter.flash_interval_millis = fi;
        current->meters.leftTop.meter.flash_interval_millis = fi;        
    }
    else  if ( t == Team::BLU ){
        current->meters.rightBottom.meter.flash_interval_millis = fi;
        current->meters.rightTop.meter.flash_interval_millis = fi;        
    }
    else{
        Log.warningln("Ignored updating flash on meters for invalid team.");
    }
}

void updateMeter (LedMeter* meter, int val, int max_val, CRGB fgColor, CRGB bgColor ){
    Log.traceln("Updating Meter, %d/%d", val, max_val);
    meter->val = val;
    meter->max_val = max_val;
    meter->fgColor = fgColor;
    meter->bgColor = bgColor;
}
void standardTeamMeters(GameState* current, bool on, FlashInterval flashing){
    int meter_val = 0;
    if ( on ){
        meter_val = STANDARD_METER_MAX_VAL;
    }
    updateMeter( &current->meters.leftTop.meter, meter_val, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
    updateMeter( &current->meters.leftBottom.meter, meter_val, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
    updateMeter( &current->meters.rightTop.meter, meter_val, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.rightBottom.meter, meter_val, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
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

    //update meters
    standardTeamMeters(current,true, FlashInterval::FLASH_NONE);    
    updateMeter( &current->meters.left.meter, red_hits, settings.hits.to_win, CRGB::Red, CRGB::Black );
    updateMeter( &current->meters.right.meter, blu_hits, settings.hits.to_win, CRGB::Blue, CRGB::Black );

    Team winner = getWinnerByHitsWithVictoryMargin(red_hits, blu_hits, settings.hits.to_win, settings.hits.victory_margin );
    Team closeToWinner = getWinnerByHitsWithoutVictoryMargin(red_hits,blu_hits,settings.hits.to_win);

    if ( winner != Team::NOBODY){
        endGame(current, winner);
    }
    else if ( current->time.overtimeExpired  ){ 
        endGameWithMostHits(current,red_hits, blu_hits);
    }
    else if ( closeToWinner != Team::NOBODY){
        Team aboutToLose = oppositeTeam(closeToWinner);
        current->status = GameStatus::GAME_STATUS_OVERTIME;
        setFlashMeterForTeam(closeToWinner, current,FlashInterval::FLASH_SLOW);
        setFlashMeterForTeam(aboutToLose, current,FlashInterval::FLASH_FAST);
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
        left            elapsed_secs        game_duration_seconds       red         black   
        left-top        10,flashing in OT   10                          red         black
        left-bottom     10,flashing in OT   10                          red         black
        center          red_hits            red_hits + blu_hits         red         blue           ( black/black when no hits yet)
        right-top       10,flashing in OT   10                          blue        black
        right           elapsed_secs        game_duration_seconds       blue        black
        right-bottom    10,flashing in OT   10                          blue        black

        
    */
    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;
    int total_hits = red_hits + blu_hits;
    int elapsed_secs = current->time.elapsed_secs;
    int game_durations_secs = settings.timed.max_duration_seconds;

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
            setFlashMeterForTeam(closeToWinner, current,FlashInterval::FLASH_FAST);
        }        
        else{
            current->status = GameStatus::GAME_STATUS_OVERTIME;    
        }
    }
    else{
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }

    //update meters
    standardTeamMeters(current,true,FlashInterval::FLASH_NONE);
    updateMeter( &current->meters.left.meter, elapsed_secs, game_durations_secs, CRGB::Red, CRGB::Black );
    updateMeter( &current->meters.right.meter, elapsed_secs, game_durations_secs, CRGB::Blue, CRGB::Black );

    if ( total_hits  > 0 ){
        updateMeter(&current->meters.center.meter, red_hits, total_hits, CRGB::Red, CRGB::Blue);
    }
    else{
        updateMeter(&current->meters.center.meter, red_hits, total_hits, CRGB::Red, CRGB::Blue);
        current->meters.center.meter.max_val = 1;
        current->meters.center.meter.val = 0;
        current->meters.center.meter.bgColor = CRGB::Black;
        current->meters.center.meter.fgColor = CRGB::Black;        
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

        METERS          val                 max-val             fgColor     bgColor
        ----------------------------------------------------------------------------
        left            blue_hits           hits_to_capture     blue         black
        left-top        10,flashing in OT   10                  blue         black
        left-bottom     10,flashing in OT   10                  blue         black
        center          blue_hits           hits_to_capture     blue         black
        right-top       10,flashing in OT   10                  blue         black
        right           blue_hits           hits_to_capture     blue         black
        right-bottom    10,flashing in OT   10                  blue         black

    */    
    int blu_hits = current->bluHits.hits;
    int hits_to_capture = settings.capture.hits_to_capture;

    if ( blu_hits >= hits_to_capture){
        endGame(current, Team::BLU);
    }
    else if ( current->time.overtimeExpired){
        endGame(current, Team::RED);
    }
    else if ( current->time.timeExpired ){
        if ( blu_hits >= (hits_to_capture - settings.hits.victory_margin)){
             current->status = GameStatus::GAME_STATUS_OVERTIME;
        }
        else{
            endGame(current,Team::RED);
        }
    }
    else {
        current->status = GameStatus::GAME_STATUS_RUNNING;
    }   

    //update meters. can't use standard meters since all are blue for AD
    updateMeter( &current->meters.leftTop.meter, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.leftBottom.meter, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.rightTop.meter, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.rightBottom.meter, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.right.meter, blu_hits, hits_to_capture, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.left.meter, blu_hits, hits_to_capture, CRGB::Blue, CRGB::Black );
    updateMeter( &current->meters.center.meter, blu_hits, hits_to_capture, CRGB::Blue, CRGB::Black );

}
void triggerOvertime(GameState* current,  GameSettings settings){
    Log.infoln("Overtime Triggered!");
    current->ownership.overtime_remaining_millis = settings.capture.capture_overtime_seconds;
}
void capture(GameState* current,  GameSettings settings){
    
    current->ownership.owner = current->ownership.capturing;
    Log.infoln("Control Point has been captured by: %c", teamTextChar(current->ownership.capturing));
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
    Log.infoln("Checking for Capture, %d/%d to capture.",current->ownership.capture_hits,settings.capture.hits_to_capture);
    if ( current->ownership.capture_hits >= settings.capture.hits_to_capture ){
        if ( current->ownership.owner != Team::NOBODY){
            triggerOvertime(current,settings);
        }
        capture(current,settings);
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
            if that's tied, the team with most hits wins
            if hit counts are tied too, the game ends in a TIE

        METERS          val                 max-val             fgColor     bgColor
        ----------------------------------------------------------------------------
        left            red_own_time        own_time_to_win     red         black
        left-top        10,flashing in OT   10                  red         black
        left-bottom     10,flashing in OT   10                  red         black
        center          capture_hits        hits_to_capture     capturing   owner(black if no owner)
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

    //update meters
    int secs_to_win = settings.timed.ownership_time_seconds;
    int red_own_secs = current->ownership.red_millis/1000;
    int blue_own_secs = current->ownership.blu_millis/1000;

    standardTeamMeters(current,true,FlashInterval::FLASH_NONE);
    updateMeter( &current->meters.left.meter, red_own_secs, secs_to_win, CRGB::Red, CRGB::Black );
    updateMeter( &current->meters.right.meter, blue_own_secs, secs_to_win, CRGB::Blue, CRGB::Black ); 

    //the center meter is the capture meter
    CRGB captureMeterBackgroundColor;
    CRGB captureMeterForegroundColor;
    captureMeterForegroundColor = getTeamColor(current->ownership.capturing);
    captureMeterBackgroundColor = getTeamColor(current->ownership.owner);

    updateMeter( &current->meters.center.meter, blue_own_secs, secs_to_win, captureMeterForegroundColor, captureMeterBackgroundColor );       
}

void updateTargetTestMode(GameState* current,  GameSettings settings, long current_time_millis){
    /* 
      This is a special game mode that's for tuning the targets.
      It never ends unless ended manually.

      It counts hits constantly, resetting the meter when necessary
      */
    int red_hits = current->redHits.hits;
    int blu_hits = current->bluHits.hits;
    Log.traceln("RedHits=%d, BluHits=%d",red_hits,blu_hits);

    //update meters
    standardTeamMeters(current,true, FlashInterval::FLASH_NONE);    

    if ( red_hits == settings.hits.to_win ){
        current->redHits.hits = 0;
    }
    if ( blu_hits == settings.hits.to_win ){
        current->bluHits.hits = 0;
    }    
    updateMeter( &current->meters.left.meter, red_hits, settings.hits.to_win, CRGB::Red, CRGB::Black );
    updateMeter( &current->meters.right.meter, blu_hits, settings.hits.to_win, CRGB::Blue, CRGB::Black );

    //never ends except manually
    current->status = GameStatus::GAME_STATUS_RUNNING;    

}
void updateMostOwnInTimeGame(GameState* current,  GameSettings settings, long current_time_millis){
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
        updateFirstToOwnTimeGame(current,settings,clock->milliseconds());
    }
    else if ( settings.gameType == GameType::GAME_TYPE_ATTACK_DEFEND){
        updateAttackDefendGame(current,settings);
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

    //very last to make sure everyone can see the time delta during update
    current->time.last_update_millis = clock->milliseconds();
}


