#ifndef __INC_STATES_H
#define __INC_STATES_H

#define DEFAULT_SHORT_RESPAWN_MILLIS 5000
#define DEFAULT_MEDIUM_RESPAWN_MILLIS 10000
#define DEFAULT_LONG_RESPAWN_MILLIS 20000

typedef enum {
    SLOT_1 = 1,
    SLOT_2 = 2,
    SLOT_3 = 3,
    SLOT_4 = 4,
    SLOT_5 = 5
} RespawnSettingSlot;

typedef struct {
    int BP_VERSION;
    int shortRespawnMillis = DEFAULT_SHORT_RESPAWN_MILLIS;
    int mediumRespawnMillis = DEFAULT_MEDIUM_RESPAWN_MILLIS;
    int longRespawnMillis = DEFAULT_LONG_RESPAWN_MILLIS;
} RespawnSettings;

#endif