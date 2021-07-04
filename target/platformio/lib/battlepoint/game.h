#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Teams.h>
#include <Clock.h>
#include <LedMeter.h>
#include <target.h>

//important: keep settings separate from 
//status, so that settings can be saved in eeprom


//////////////////////////////////////
//Settings Things
//////////////////////////////////////
typedef struct {
    int game_duration_seconds;
} TimedGame;

typedef enum {
    GAME_TYPE_KOTH_FIRST_TO_HITS,
    GAME_TYPE_KOTH_MOST_HITS_IN_TIME,
    GAME_TYPE_KOTH_FIRST_TO_OWN_TIME,
    GAME_TYPE_KOTH_MOST_OWN_IN_TIME,
    GAME_TYPE_ATTACK_DEFEND
} GameType;

typedef struct{
    LedMeter leftTop;
    LedMeter leftBottom;
    LedMeter rightTop;
    LedMeter rightBottom;
    LedMeter center;
    LedMeter left;
    LedMeter right;
} Meters;

typedef struct{
    int hits_to_capture;
    int capture_cooldown_seconds;
    int capture_decay_rate_secs_per_hit;
    int capture_offense_to_defense_ratio;
} CaptureSettings;

typedef struct{
    GameType gameType;
    TargetSettings targetSettings;
    int hitsToWin= 0;
    int mustWinBy = 0;
} FirstToHitsGameSettings;

typedef struct{
    GameType gameType;
    TimedGame gameTimeSettings;    
    TargetSettings targetSettings;
} MostHitsInTimeGameSettings;

typedef struct{
    GameType gameType;
    TimedGame gameTimeSettings;
    TargetSettings targetSettings;
    CaptureSettings captureSettings;
} MostOwnInTimeGameSettings;

typedef struct{
    GameType gameType;
    TimedGame gameTimeSettings;
    TargetSettings targetSettings;
    CaptureSettings captureSettings;    
} AttackDefendGameSettings;

typedef struct{
    GameType gameType;
    TimedGame gameTimeSettings;
    TargetSettings targetSettings;
    CaptureSettings captureSettings;    
} FirstToOwnTimeGameSettings;

//////////////////////////////////////
///  Status Things
//////////////////////////////////////
typedef enum {
    GAME_STATUS_OVERTIME,
    GAME_STATUS_RUNNING,
    GAME_STATUS_ENDED
} GameStatus;

typedef struct {
    long start_time_millis;
    long last_update_millis;
} GameTime;

typedef struct {
    Team winner;
} GameResult;

typedef struct {
    long blu_millis;
    long red_millis;
    Team capturing;
    int capture_hits;
    Team owner;
} Ownership;

typedef struct {
    int blu_hits;
    int red_hits;
} TeamHits;

typedef struct{
    GameStatus status;
    GameTime time;
    GameResult result;
    TeamHits hits;
    FirstToHitsGameSettings settings;
} GameFirstToHits;

typedef struct{
    GameStatus status;    
    GameTime time;
    GameResult result;
    TeamHits hits;
    MostHitsInTimeGameSettings settings;
} GameMostHitsInTime;

typedef struct{
    GameStatus status;    
    GameTime time;
    GameResult result;
    Ownership ownership;
    TeamHits hits;
    FirstToOwnTimeGameSettings settings;
} GameFirstToOwnTime;

typedef struct{
    GameStatus status;    
    GameTime time;
    GameResult result;
    Ownership ownership;
    MostOwnInTimeGameSettings settings;
} GameMostOwnInTime;

typedef struct{
    GameStatus status;    
    GameTime time;
    GameResult result;
    TeamHits hits;
    AttackDefendGameSettings settings;
} GameAttackDefend;


//game updates
//GameAttackDefend update(GameAttackDefend, Clock* clock );
//GameMostOwnInTime update()
//GameFirstToOwnTime
//GameMostHitsInTime
GameFirstToHits update(GameFirstToHits current, Clock* clock);

//GameFirstToHits update(GameFirstToHits current_game, long millis_since_last_update);
//GameMostHitsInTime update(GameMostHitsInTime current_game, long millis_since_last_update);
//GameFirstToOwnTime update(GameFirstToOwnTime current_game, long millis_since_last_update);
//GameMostOwnInTime update(GameMostOwnInTime current_game, long millis_since_last_update);

//computation components


//GameTime _updateGametime(GameTime current, Clock* clock);
Ownership compute_ownership_time( Ownership current, GameTime current_time, long millis_since_game_start);
#endif