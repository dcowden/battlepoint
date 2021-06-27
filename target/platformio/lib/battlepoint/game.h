
#include "Teams.h"

typedef enum {
    GAME_STATUS_OVERTIME,
    GAME_STATUS_RUNNING,
    GAME_STATUS_ENDED
} GameStatus;

typedef enum {
    GAME_TYPE_KOTH_FIRST_TO_HITS,
    GAME_TYPE_KOTH_MOST_HITS_IN_TIME,
    GAME_TYPE_KOTH_FIRST_TO_OWN_TIME,
    GAME_TYPE_KOTH_MOST_OWN_IN_TIME
} GameType;

typedef struct {
    long start_time_millis;
    long last_update_millis;
    long est_end_time_millis;
} GameTime;

typedef struct {
    Team winner;
} GameResult;

typedef struct {
    long blu_millis;
    long red_millis;
    Team capturing;
    char capture_hits;
    Team owner;
} Ownership;

typedef struct {
    int blu_hits;
    int red_hits;
} TeamHits;

typedef struct{
    int game_duration_seconds;
} TimedGame;

typedef struct{
    int shield_time_seconds;
    int shield_cooldown_seconds;
    int shield_cooldown_reduction_seconds;
} ShieldSettings;

typedef struct{
    GameTime time;
    GameResult result;
    ShieldSettings shieldsettings;
    TeamHits hits;
    int hits_to_win= 0;

} GameFirstToHits;

typedef struct{
    GameTime time;
    GameResult result;
    ShieldSettings shieldsettings;
    TimedGame settings;
    TeamHits hits;
} GameMostHitsInTime;

typedef struct{
    GameTime time;
    GameResult result;
    ShieldSettings shieldsettings;
    int win_time_seconds = 0;
    Ownership ownership;
    TeamHits hits;
} GameFirstToOwnTime;

typedef struct{
    GameTime time;
    GameResult result;
    ShieldSettings shieldsettings;
    TimedGame settings;
    Ownership ownership;
} GameMostOwnInTime;

GameFirstToHits update(GameFirstToHits current_game, long millis_since_last_update);
GameMostHitsInTime update(GameMostHitsInTime current_game, long millis_since_last_update);
GameFirstToOwnTime update(GameFirstToOwnTime current_game, long millis_since_last_update);
GameMostOwnInTime update(GameMostOwnInTime current_game, long millis_since_last_update);

long seconds_to_millis(int seconds);
char char_for_team(Team t);
bool should_game_end(TimedGame timedGame, long millis_since_game_start);
GameTime compute_game_time(GameTime current, long millis_since_game_start);
Ownership compute_ownership_time( Ownership current, GameTime current_time, long millis_since_game_start);