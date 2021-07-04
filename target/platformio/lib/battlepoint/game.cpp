#include <game.h>
#include <Teams.h>
#include <Clock.h>
#include <math.h>

GameFirstToHits update(GameFirstToHits current, Clock* clock){
    GameFirstToHits updated = current;
    updated.time = _updateGameTime(updated.time, clock);

    if ( current.hits.blu_hits >= current.settings.hitsToWin ){
        updated.result.winner = Team::BLU;
        updated.status = GameStatus::GAME_STATUS_ENDED;
    }
    else if ( current.hits.red_hits >= current.settings.hitsToWin){
        updated.result.winner = Team::RED;
        updated.status = GameStatus::GAME_STATUS_ENDED;
    }
    else if ( abs(current.hits.red_hits - current.hits.blu_hits) < current.settings.mustWinBy ){
        updated.status == GameStatus::GAME_STATUS_OVERTIME;
    }
    else{
        updated.status = GameStatus::GAME_STATUS_RUNNING;
    }
    return updated;
}


GameTime _updateGameTime(GameTime current, Clock* clock){
    GameTime updated = current;
    updated.last_update_millis = clock->milliseconds();
    return updated;
}

Ownership compute_ownership_time( Ownership current, GameTime current_time, long millis_since_game_start){
    Ownership new_ownership = current;
    long elapsed_millis = millis_since_game_start - current_time.last_update_millis;

    if ( current.owner == Team::BLU ){
        new_ownership.blu_millis += elapsed_millis;
    }
    else if ( current.owner == Team::RED ){
        new_ownership.red_millis += elapsed_millis;
    }
    return new_ownership;
}

/**
Game update_game(Game current_game, long millis_since_game_start){

    Game new_game = current_game;
    GameStatus new_status = new_game.status;
    GameStatus current_status = current_game.status;
    new_status.last_update_millis = millis_since_game_start;

    //first to hits


    //most hits in time
    if ( current_game.type == GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){

        if ( should_game_end( current_game.settings.game_duration_seconds, millis_since_game_start) ){
            if ( current_status.blu_total_hits > current_status.red_total_hits){
                end_game_with_winner(&new_status,TEAM_BLU,millis_since_game_start);
            }
            else if ( current_status.red_total_hits > current_status.blu_total_hits){
                end_game_with_winner(&new_status,TEAM_RED,millis_since_game_start);
            }
            else{
                new_status.overtime=true;
            }
        }

    }

    //first_to_own_time
    if ( current_game.type == GAME_TYPE_KOTH_MOST_HITS_IN_TIME ){
        long elapsed_millis = millis_since_game_start - current_status.last_update_millis;
        increment_ownership_time(&current_status,millis_since_game_start);

        if ( new_status.blu_own_millis > seconds_to_millis(current_game.settings.win_time_seconds) ){
            end_game_with_winner(&new_status,TEAM_BLU,millis_since_game_start);
        }
        else if ( new_status.red_own_millis > seconds_to_millis(current_game.settings.win_time_seconds) ){
            end_game_with_winner(&new_status,TEAM_RED,millis_since_game_start);
        }

    }

    //most own in time
    if ( current_game.type == GAME_TYPE_KOTH_MOST_OWN_IN_TIME ){
         if ( should_game_end( current_game.settings.game_duration_seconds, millis_since_game_start) ){
            increment_ownership_time(&new_status,millis_since_game_start);

            if ( new_status.blu_own_millis > new_status.red_own_millis){
                end_game_with_winner(&new_status,TEAM_BLU,millis_since_game_start);
            }
            else if (new_status.red_own_millis > new_status.blu_own_millis){
                end_game_with_winner(&new_status,TEAM_RED,millis_since_game_start);
            }
            else{
                new_status.overtime=true;
            }
         }
    }

    Game new_game;
    new_game.settings = current_game.settings;
    new_game.type = current_game.type;
    
    new_game.status = new_status;
    return new_game;
} **/
