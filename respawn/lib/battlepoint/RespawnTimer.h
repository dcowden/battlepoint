#ifndef __INC_RESPAWNTIMER_H
#define __INC_RESPAWNTIMER_H
//#include <Arduino.h>

typedef enum {
    RESPAWNING = 0,
    IMMINENT =1,
    FINISHED = 2,
    IDLE=3
} RespawnTimerState;

const int MILLIS_NOT_STARTED = -1;
const int DEFAULT_READY_TIME_LEFT_MILLIS =3000;
const int DEFAULT_GO_SIGNAL_MILLIS =3000;

class RespawnTimer{
    public:
        RespawnTimer(){
            disable();
        };

        virtual bool isIdle( long currentTimeMillis){
            return (computeTimerState( currentTimeMillis)) == RespawnTimerState::IDLE;
        };
        virtual void update(long currentTimeMillis){

        };
        virtual void start(long timerDurationMillis, long currentTimeMillis){
            durationMillis = timerDurationMillis;
            startMillis = currentTimeMillis;
            endMillis = currentTimeMillis + timerDurationMillis;
            respawnImminentMillis = endMillis - DEFAULT_READY_TIME_LEFT_MILLIS;
            resetMillis = endMillis + DEFAULT_GO_SIGNAL_MILLIS; 
        };

        virtual void disable(){
            startMillis = MILLIS_NOT_STARTED;
            endMillis = MILLIS_NOT_STARTED;
            resetMillis = MILLIS_NOT_STARTED;
            respawnImminentMillis = MILLIS_NOT_STARTED;
        };
        virtual RespawnTimerState computeTimerState( long currentTimeMillis){
            if ( startMillis == MILLIS_NOT_STARTED){
                return RespawnTimerState::IDLE;
            }
            else{
                long timeLeftMillis = endMillis - currentTimeMillis;

                if ( timeLeftMillis > 0 ){
                    if ( currentTimeMillis < respawnImminentMillis){
                        return RespawnTimerState::RESPAWNING;
                    }
                    else{
                        return RespawnTimerState::IMMINENT;
                    }
                }
                else {
                    // no time left
                    if ( currentTimeMillis > resetMillis ){
                        return RespawnTimerState::IDLE;
                    }
                    else{
                        return RespawnTimerState::FINISHED;
                    }
                }
            }
        };
    protected:
        long durationMillis;
        long startMillis;
        long endMillis;
        long respawnImminentMillis;
        long resetMillis;

};
#endif
