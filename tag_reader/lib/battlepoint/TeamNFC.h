#pragma once


const long START_TIME_NOTSTARTED = -1;
const int NOT_SET = 1;

typedef enum {
    T_RED='R',
    T_BLUE='B',
    T_NONE='N'
} TeamChar;

typedef enum{
    CLASS=1,
    FLAG=3,
    MEDIC=4,
    NONE=5,
    UNKNOWN=6
} NFCCardType;

typedef enum {
    DEAD=0,
    RESPAWNING=1,
    ALIVE=2,
    INVULNERABLE=3,
    INIT
} LifeStage;

typedef enum{
    PC_SCOUT ='C',
    PC_MEDIC='M',
    PC_HEAVY='H',
    PC_SOLDIER='O',
    PC_SNIPER='N',
    PC_ANONYMOUS='A'
} PlayerClass;

typedef struct { 
    //TODO: divide into a class or nest these?
    //configurations
    int max_hp;
    int big_hit=NOT_SET;
    int little_hit=NOT_SET;
    int invuln_ms=NOT_SET;
    long respawn_ms=NOT_SET;
    unsigned char team = TeamChar::T_NONE;
    unsigned char player_class = PlayerClass::PC_ANONYMOUS;

    //state
    long respawn_allowed_time_ms=START_TIME_NOTSTARTED;
    long invuln_end_time_ms = START_TIME_NOTSTARTED;
    int hp = 0;
    int state = LifeStage::INIT;    
    bool respawnRequested = false; 

} LifeConfig;


typedef struct {
    int max_hp;
    int big_hit;
    int little_hit;
    long invuln_ms;
    long respawn_ms;
    char team;
    int player_class;
} ClassCard;

typedef struct {
    int hp;
} MedicCard;

typedef struct {
    int value;
    char team;
} FlagCard;

//note: this structure has to hold all the types of cards.
//so a union is ideal here to show the intent ( saves memory too but that's less important)
typedef struct {
    int type= NFCCardType::NONE;
    union{
        MedicCard medicCard;
        FlagCard flagCard;
        ClassCard classCard;
    };
} NFCCard;

void setDefaults( NFCCard &card);
void setDefaults( ClassCard &card);
void setDefaults( MedicCard &card);
void setDefaults( FlagCard &card);
void requestRespawn(LifeConfig &config , long current_time_millis);
void start_invuln(LifeConfig &config , long current_time_millis);
void end_invuln(LifeConfig &config , long current_time_millis);
void updateLifeStatus(LifeConfig &config, long current_time_millis);
const char * lifeStatus( LifeConfig &config );
void medic(LifeConfig &config, int add_hp, long current_time_millis);
void big_hit(LifeConfig &config, long current_time_millis);
void little_hit(LifeConfig &config, long current_time_millis);