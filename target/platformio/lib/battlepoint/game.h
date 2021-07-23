#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Teams.h>
#include <Clock.h>
#include <LedMeter.h>
#include <target.h>
#define BP_CURRENT_SETTINGS_VERSION 203

//important: keep settings separate from 
//status, so that settings can be saved in eeprom



//////////////////////////////////////
//Settings Things
//////////////////////////////////////
typedef struct {
    int max_duration_seconds;
    int ownership_time_seconds;
} TimedGame;

typedef enum {
    SLOT_1 = 1,
    SLOT_2 = 2,
    SLOT_3 = 3,
    SLOT_4 = 4
} GameSettingSlot;


typedef enum {
    GAME_TYPE_KOTH_FIRST_TO_HITS=0,
    GAME_TYPE_KOTH_MOST_HITS_IN_TIME=1,
    GAME_TYPE_KOTH_FIRST_TO_OWN_TIME=2,
    GAME_TYPE_KOTH_MOST_OWN_IN_TIME=3,
    GAME_TYPE_ATTACK_DEFEND=4,
    GAME_TYPE_UNSELECTED=5
} GameType;

typedef struct {
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
    int BP_VERSION;
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
    GAME_STATUS_OVERTIME='O',
    GAME_STATUS_RUNNING = 'R',
    GAME_STATUS_ENDED= 'E'
} GameStatus;

typedef struct{
    //TODO: should these be composed?
    LedController leftTop;
    LedController leftBottom;
    LedController rightTop;
    LedController rightBottom;
    LedController center;
    LedController left;
    LedController right;
} MeterSettings;

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

GameSettings DEFAULT_GAMESETTINGS();
GameState startGame(GameSettings settings, Clock* clock);
void updateGame(GameState* game, SensorState sensors, GameSettings settings, Clock* clock);
const char* getCharForGameType(GameType t);
const char* getCharForStatus(GameStatus s);
#endif