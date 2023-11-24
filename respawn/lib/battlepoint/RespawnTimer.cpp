#include <RespawnTimer.h>
#include <ArduinoLog.h>

void startTimer ( RespawnTimer* timer, int timerDurationMillis, long currentTimeMillis){
    if ( timer->startMillis)
    timer->durationMillis = timerDurationMillis;
    timer->startMillis = currentTimeMillis;
    timer->endMillis = currentTimeMillis + timerDurationMillis;
    timer->respawnImminentMillis = timer->endMillis - DEFAULT_READY_TIME_LEFT_MILLIS;
    timer->resetMillis = timer->endMillis + DEFAULT_GO_SIGNAL_MILLIS;    
}

bool isAvailable(RespawnTimer* timer, long currentTimeMillis ){
    RespawnTimerState s;
    s = computeTimerState( timer, currentTimeMillis);
    return s == RespawnTimerState::IDLE;
}
void disableTimer ( RespawnTimer* timer){
    timer->startMillis = MILLIS_NOT_STARTED;
    timer->endMillis = MILLIS_NOT_STARTED;
    timer->resetMillis = MILLIS_NOT_STARTED;
    timer->respawnImminentMillis = MILLIS_NOT_STARTED;
}

RespawnTimerState computeTimerState (RespawnTimer* timer, long currentTimeMillis){
    if ( timer->startMillis == MILLIS_NOT_STARTED){
        return RespawnTimerState::IDLE;
    }
    else{
        long timeLeftMillis = timer->endMillis - currentTimeMillis;

        if ( timeLeftMillis > 0 ){
            if ( currentTimeMillis < timer->respawnImminentMillis){
                return RespawnTimerState::RESPAWNING;
            }
            else{
                return RespawnTimerState::IMMINENT;
            }
        }
        else {
            // no time left
            if ( currentTimeMillis > timer->resetMillis ){
                return RespawnTimerState::IDLE;
            }
            else{
                return RespawnTimerState::FINISHED;
            }
        }
    }

}