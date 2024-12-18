#pragma once

const long START_TIME_NOTSTARTED = -1;
const int NOT_SET = 1;

typedef enum {
    T_RED=0,
    T_BLUE=1,
    T_NONE=2
} TeamChar;

typedef enum{
    CLASS=1,
    FLAG=3,
    MEDIC=4,
    NONE=5,
    UNKNOWN=6
} NFCCardType;



typedef enum{
    PC_SCOUT =0,
    PC_MEDIC=1,
    PC_HEAVY=2,
    PC_SOLDIER=3,
    PC_SNIPER=4,
    PC_ANONYMOUS=5
} PlayerClass;

typedef struct {
    int max_hp;
    int big_hit;
    int little_hit;
    long invuln_ms;
    long respawn_ms;
    char team;
    char player_class;
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

void cardSetDefaults( NFCCard &card);
void cardSetDefaults( ClassCard &card);
void cardSetDefaults( MedicCard &card);
void cardSetDefaults( FlagCard &card);