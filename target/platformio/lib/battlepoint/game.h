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

typedef struct{
    LedMeter leftTop;
    LedMeter leftBottom;
    LedMeter rightTop;
    LedMeter rightBottom;
    LedMeter center;
    LedMeter left;
    LedMeter right;
} MeterSettings;

typedef enum {
    GAME_TYPE_KOTH_FIRST_TO_HITS,
    GAME_TYPE_KOTH_MOST_HITS_IN_TIME,
    GAME_TYPE_KOTH_FIRST_TO_OWN_TIME,
    GAME_TYPE_KOTH_MOST_OWN_IN_TIME,
    GAME_TYPE_ATTACK_DEFEND
} GameType;

typedef struct {
    int to_capture;
    int to_win;
    int victory_margin;
} HitCounts;

typedef struct{
    int hits_to_capture;
    int capture_cooldown_seconds;
    int capture_decay_rate_secs_per_hit;
    int capture_offense_to_defense_ratio;
} CaptureSettings;

typedef struct {    
    GameType gameType;
    TargetSettings target;
    TimedGame timed;
    CaptureSettings capture;
    HitCounts hits;
} GameSettings;


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
    int blu_hits;
    int red_hits;
} TeamHits;

typedef struct {
    long blu_millis;
    long red_millis;
    Team capturing;
    int capture_hits;
    Team owner;
} Ownership;

typedef struct {
    GameStatus status;
    GameResult result;
    GameTime time;
    TeamHits hits;
    Ownership ownership;
    MeterSettings meters;
} GameState;

GameState startGame(GameSettings settings, Clock* clock);
GameState updateGame(GameState current, SensorState sensors, GameSettings settings, Clock* clock);


#endif