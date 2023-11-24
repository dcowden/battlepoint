#ifndef __INC_RESPAWNTIMER_H
#define __INC_RESPAWNTIMER_H

typedef enum {
    RESPAWNING = 0,
    IMMINENT =1,
    FINISHED = 2,
    IDLE=3
} RespawnTimerState;

const int DEFAULT_READY_TIME_LEFT_MILLIS =3000;
const int DEFAULT_GO_SIGNAL_MILLIS =3000;

typedef struct  {
    int id=0;
    long durationMillis=0;
    long startMillis=0;
    long endMillis=0;
    long respawnImminentMillis=0;
    long resetMillis=0;
} RespawnTimer;

bool isAvailable(RespawnTimer* timer, long currentTimeMillis );
RespawnTimerState computeTimerState (RespawnTimer* timer, long currentTimeMillis);
void startTimer ( RespawnTimer* timer, int durationMillis, long currentTimeMillis );

#endif
