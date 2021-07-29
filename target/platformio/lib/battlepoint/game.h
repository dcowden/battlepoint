#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Teams.h>
#include <Clock.h>
#include <LedMeter.h>
#include <target.h>
#define BP_CURRENT_SETTINGS_VERSION 203
#define STANDARD_METER_MAX_VAL 10
//important: keep settings separate from 
//status, so that settings can be saved in eeprom



//////////////////////////////////////
//Settings Things
//////////////////////////////////////
typedef struct {
    int max_duration_seconds;
    int max_overtime_seconds;
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
    int capture_overtime_seconds;
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
    long start_time_millis=0;
    long last_update_millis=0;
    bool timeExpired = false;
    bool overtimeExpired = false;
    int elapsed_secs = 0;
} GameTime;

typedef struct {
    Team winner;
} GameResult;

//for hit counting games each team has one
//for capturing games, there is only one
typedef struct {
    int hits=0;
    long last_hit_millis=0;
    long last_decay_millis=0;
} HitTracker;

typedef struct {
    long blu_millis=0;
    long red_millis=0;
    Team capturing = Team::NOBODY;
    int capture_hits = 0;
    Team owner = Team::NOBODY;
    long overtime_remaining_millis = 0;
    long last_hit_millis=0;
    long last_decay_millis=0;
} Ownership;

typedef struct {
    GameStatus status;
    GameResult result;
    GameTime time;
    HitTracker redHits; 
    HitTracker bluHits;
    Ownership ownership;
    MeterSettings meters;
} GameState;

GameSettings DEFAULT_GAMESETTINGS();
GameState startGame(GameSettings settings, Clock* clock,MeterSettings base_meters);

//exposed for testing
void updateGameTime(GameState* current,GameSettings settings, long current_time_millis);
void updateOwnership(GameState* current,  GameSettings settings, long current_time_millis);
void applyHitDecay(GameState* current, GameSettings settings, long current_time_millis);
void updateGameHits(GameState* current, SensorState sensors, long current_time_millis);
void updateFirstToHitsGame(GameState* current,  GameSettings settings);
void updateMostHitsInTimeGame(GameState* current,  GameSettings settings);
void updateFirstToOwnTimeGame(GameState* current,  GameSettings settings, long current_time_millis);
void updateAttackDefendGame(GameState* current,  GameSettings settings);
void updateMostOwnInTimeGame(GameState* current,  GameSettings settings, long current_time_millis);
void updateGame(GameState* game, SensorState sensors, GameSettings settings, Clock* clock);
void updateLeds(GameState* current, long current_time_millis );


const char* getCharForGameType(GameType t);
const char* getCharForStatus(GameStatus s);
#endif