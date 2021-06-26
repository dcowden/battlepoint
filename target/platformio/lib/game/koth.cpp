#include <koth.h>

char char_for_team(Team t){
    if ( t == Team::BLU){
        return 'B';
    }
    else if ( t == Team::RED){
        return 'R';
    }
    else if ( t == Team::NOBODY){
        return 'X';
    }
    else if ( t == Team::EVERYBODY){
        return 'B';
    }
    else{
        return '?';
    }
}

long seconds_to_millis(int seconds ){
    return 1000*seconds;
}

bool should_game_end(TimedGame timedGame, long millis_since_game_start){
    return  millis_since_game_start >= seconds_to_millis(timedGame.game_duration_seconds);
}

GameMostOwnInTime update(GameMostOwnInTime current_game, long millis_since_last_update);

GameTime compute_game_time(GameTime current, long millis_since_game_start){
    GameTime new_game = current;
    new_game.last_update_millis = millis_since_game_start;
    return new_game;
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

GameFirstToHits update(GameFirstToHits current_game, long millis_since_last_update){
    GameFirstToHits new_game = current_game;
    new_game.time = compute_game_time(current_game.time,millis_since_last_update);
    if ( current_game.hits.blu_hits > current_game.hits_to_win ){
        new_game.result = { Team::BLU };
    }

    else if ( current_game.hits.red_hits > current_game.hits_to_win ) {
        new_game.result = { Team::RED };
    }
    else {
        new_game.result = { Team::NOBODY };
    }
    return new_game;
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
