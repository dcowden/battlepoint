#ifndef __INC_STATES_H
#define __INC_STATES_H

typedef enum {
    SLOT_1 = 1,
    SLOT_2 = 2,
    SLOT_3 = 3,
    SLOT_4 = 4,
    SLOT_5 = 5
} RespawnSettingSlot;

typedef struct {
    int BP_VERSION;
    int respawn_time_seconds = 10;
} RespawnSettings;

#endif