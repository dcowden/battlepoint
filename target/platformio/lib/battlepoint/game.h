#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Teams.h>
#include <LedMeter.h>
#include <target.h>
#include <targetscan.h>

#define STANDARD_METER_MAX_VAL 10
//important: keep settings separate from 
//status, so that settings can be saved in eeprom


//////////////////////////////////////
//Settings Things
//////////////////////////////////////
typedef struct {
    int max_duration_seconds = 240;
    int max_overtime_seconds = 30;
    int ownership_time_seconds = 30;
    int countdown_start_seconds = 10;
} TimedGame;

typedef enum {
    SLOT_1 = 1,
    SLOT_2 = 2,
    SLOT_3 = 3,
    SLOT_4 = 4,
    SLOT_5 = 5
} GameSettingSlot;



typedef enum {
    GAME_TYPE_KOTH_FIRST_TO_HITS=0,
    GAME_TYPE_KOTH_MOST_HITS_IN_TIME=1,
    GAME_TYPE_KOTH_FIRST_TO_OWN_TIME=2,
    GAME_TYPE_KOTH_MOST_OWN_IN_TIME=3,
    GAME_TYPE_ATTACK_DEFEND=4,
    GAME_TYPE_TARGET_TEST=5,
    GAME_TYPE_UNSELECTED=6,
} GameType;

typedef struct {
    int to_win = 10;
    int victory_margin = 2;
} HitCounts;

typedef struct{
    int hits_to_capture = 15;
    int capture_cooldown_seconds = 1;
    int capture_decay_rate_secs_per_hit = 5;
    int capture_overtime_seconds= 30;
} CaptureSettings;

typedef struct {
    long last_hit_millis = 0;
    long trigger_threshold = 2000;
    long hit_energy_threshold = 7;
} TargetSettings;

typedef struct {    
    int BP_VERSION;
    GameType gameType = GAME_TYPE_UNSELECTED;
    TargetSettings target;
    TimedGame timed;
    CaptureSettings capture;
    HitCounts hits;
} GameSettings;



//////////////////////////////////////
///  Status Things
//////////////////////////////////////
typedef enum {
    GAME_STATUS_PREGAME='C',
    GAME_STATUS_OVERTIME='O',
    GAME_STATUS_RUNNING = 'R',
    GAME_STATUS_ENDED= 'E'
} GameStatus;

typedef struct{
    LedMeter* leftTop;
    LedMeter* leftBottom;
    LedMeter* rightTop;
    LedMeter* rightBottom;
    LedMeter* center;
    LedMeter* left;
    LedMeter* right;
} MeterSettings;

typedef struct {
    long start_time_millis=0; //start of the whole game, including pre-game countdown
    long last_update_millis=0;
    bool timeExpired = false;
    bool overtimeExpired = false;
    int remaining_secs = 0; //time left in the game if the game has startedd. time left till game start in the pregame  
    int elapsed_secs =  0; //time relative to game start. <0 means pre-game countdown. 
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
    long last_hit_energy=0;
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

typedef   void (*endedHandler)(Team);
typedef   void (*startedHandler)(GameStatus);
typedef   void (*overtimeHandler)(void);
typedef   void (*capturedHandler)(Team);
typedef   void (*contestedHandler)(void);
typedef   void (*cancelledHandler)(void);
typedef   void (*remainingsecsHandler)(int,GameStatus);

typedef struct {
   endedHandler EndedHandler;
   startedHandler StartedHandler;
   overtimeHandler OvertimeHandler;
   capturedHandler CapturedHandler;
   contestedHandler ContestedHandler;
   cancelledHandler CancelledHandler;
   remainingsecsHandler RemainingSecsHandler;
} EventHandler;

typedef struct {
    GameStatus status;
    GameResult result;
    GameTime time;
    HitTracker redHits; 
    HitTracker bluHits;
    Ownership ownership;
    TargetHitData lastHit;
    EventHandler eventHandler;
} GameState;

void setDefaultGameSettings(GameSettings* settings );
void startGame(GameState* gs, GameSettings* settings, long current_time_millis);
void gamestate_init(GameState* state);

//exposed for testing
void updateGameTime(GameState* current,GameSettings settings, long current_time_millis);
void updateOwnership(GameState* current,  GameSettings settings, long current_time_millis);
void applyHitDecay(GameState* current, GameSettings settings, long current_time_millis);

void applyLeftHits(GameState* current, GameSettings* settings,TargetHitData hitdata, long current_time_millis);
void applyRightHits(GameState* current, GameSettings* settings,TargetHitData hitdata, long current_time_millis);
void updateFirstToHitsGame(GameState* current,  GameSettings settings);
void updateMostHitsInTimeGame(GameState* current,  GameSettings settings);
void updateFirstToOwnTimeGame(GameState* current,  GameSettings settings, long current_time_millis);
void updateAttackDefendGame(GameState* current,  GameSettings settings, long current_time_millis);
void updateMostOwnInTimeGame(GameState* current,  GameSettings settings, long current_time_millis);
void updateGame(GameState* game, GameSettings settings, long current_time_millis);
void updateLeds(MeterSettings* meters, long current_time_millis );
void updateMeters(GameState* game, GameSettings* settings, MeterSettings* meters);

const char* getCharForGameType(GameType t);
const char* getCharForStatus(GameStatus s);
#endif