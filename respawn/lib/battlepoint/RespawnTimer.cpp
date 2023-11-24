#include <RespawnTimer.h>
#include <ArduinoLog.h>

void startTimer ( RespawnTimer* timer, int timerDurationMillis, long currentTimeMillis){
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

RespawnTimerState computeTimerState (RespawnTimer* timer, long currentTimeMillis){
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