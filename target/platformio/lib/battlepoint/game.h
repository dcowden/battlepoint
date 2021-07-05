#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Teams.h>
#include <Clock.h>
#include <LedMeter.h>
#include <target.h>

//important: keep settings separate from 
//status, so that settings can be saved in eeprom

typedef struct{
    LedMeter leftTop;
    LedMeter leftBottom;
    LedMeter rightTop;
    LedMeter rightBottom;
    LedMeter center;
    LedMeter left;
    LedMeter right;
} Meters;

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

typedef struct {
    GameType gameType;
    TargetSettings targetSettings;
} BaseGameSettings;

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
    int blu_hits;
    int red_hits;
} TeamHits;

typedef struct {
    GameStatus status;
    GameTime time;
    GameResult result;
    TeamHits hits;
} BaseGameState;

typedef struct {
    long blu_millis;
    long red_millis;
    Team capturing;
    int capture_hits;
    Team owner;
} Ownership;

typedef struct{
    BaseGameState state;
    FirstToHitsGameSettings settings;
} FirstToHitsGame;

typedef struct{
    BaseGameState state;
    MostHitsInTimeGameSettings settings;
} MostHitsInTimeGame;

typedef struct{
    BaseGameState state;
    Ownership ownership;
    FirstToOwnTimeGameSettings settings;
} FirstToOwnTimeGame;

typedef struct{
    BaseGameState state;
    Ownership ownership;
    MostOwnInTimeGameSettings settings;
} MostOwnInTimeGame;

typedef struct{
    BaseGameState state;
    AttackDefendGameSettings settings;
} AttackDefendGame;

//common start methods
void initGame(BaseGameState* state, Clock* clock);
void initOwnership(Ownership* ownership, Clock* clock);

//common update methods
void updateTeamHits( TeamHits* hitsToUpdate, TargetHitScanResult leftScan, TargetHitScanResult rightScan ,Clock* clock);
void updateOwnership(Ownership* ownership, GameTime current_time, Clock* clock);


//game specific methods
void startGame(FirstToHitsGame* game, Clock* clock);
void update(FirstToHitsGame* game, TargetHitScanResult leftScan, TargetHitScanResult rightScan, Clock* clock);


#endif