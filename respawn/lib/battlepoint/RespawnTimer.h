#ifndef __INC_RESPAWNTIMER_H
#define __INC_RESPAWNTIMER_H

typedef enum {
    RESPAWNING = 0,
    IMMINENT =1,
    FINISHED = 2,
    IDLE=3
} RespawnTimerState;

const int MILLIS_NOT_STARTED = -1;
const int DEFAULT_READY_TIME_LEFT_MILLIS =3000;
const int DEFAULT_GO_SIGNAL_MILLIS =3000;

typedef struct  {
    int id;
    long durationMillis;
    long startMillis;
    long endMillis;
    long respawnImminentMillis;
    long resetMillis;
} RespawnTimer;

bool isAvailable(RespawnTimer* timer, long currentTimeMillis );
RespawnTimerState computeTimerState (RespawnTimer* timer, long currentTimeMillis);
void startTimer ( RespawnTimer* timer, int durationMillis, long currentTimeMillis );
void disableTimer ( RespawnTimer* timer);
#endif
