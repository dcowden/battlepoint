#ifndef __INC_RESPAWNTIMER_H
#define __INC_RESPAWNTIMER_H
#include <Clock.h>

typedef enum {
    RESPAWN_START = 0,
    RESPAWNING = 1,
    IMMINENT =2,
    FINISHED = 3,
    IDLE = 4
} RespawnTimerState;

typedef struct{
    long durationMillis;  //how long is the respawn interval?
    long immientMillis;   //how long before completion do we get the 'get ready' state?
    long afterFinishMillis; //how long after finish do we indicate the go signal?
    long afterStartMillis;  //how long after start do we indicate the respawning signal?
} TimerConfig;

const int MILLIS_NOT_STARTED = -1;

typedef void (*stateCallback)(RespawnTimerState newState, int id);

//TODO, this would be cool as an operator on TimerConfig
//inline RespawnTimerState& computeTimerState (long timeSinceStart ) __attribute__((always_inline))
RespawnTimerState computeStateForTimer( TimerConfig config, long elapsedMillis){
    //compute some relative end times
    long endTimeMillis = config.durationMillis;
    long timeWhenImminentStarts = config.durationMillis - config.immientMillis;
    long endOfPostFinish = config.durationMillis + config.afterFinishMillis;

    if ( elapsedMillis < 0 ){
        //invalid state
        return RespawnTimerState::IDLE;
    }
    if ( elapsedMillis <  config.afterStartMillis){
        return RespawnTimerState::RESPAWN_START;
    }
    else if ( elapsedMillis < timeWhenImminentStarts ){
        return RespawnTimerState::RESPAWNING;
    }
    else if ( elapsedMillis < endTimeMillis){
        return RespawnTimerState::IMMINENT;
    }
    else if ( elapsedMillis < endOfPostFinish){
        return RespawnTimerState::FINISHED;
    }
    else{
        return RespawnTimerState::IDLE;
    }
};

class RespawnTimer{
    public:
        RespawnTimer(Clock* clock, int id){
            _clock = clock;
            _id = id;
            _running = false;
        };

        bool isAvailable(){
            return (state() == RespawnTimerState::FINISHED || state() == RespawnTimerState::IDLE);
        }
        int id(){
            return _id;
        };

        void onStateChange( stateCallback callback){
            _callback = callback;
        };

        void start(TimerConfig config){
            _config = config;
            _startMillis= _clock->milliseconds();      
            _running = true;
            _lastState = RespawnTimerState::IDLE;
            updateState(RespawnTimerState::RESPAWN_START);
        };

        void stop(){
            _running = false;
            updateState(RespawnTimerState::IDLE);
        };

        RespawnTimerState state(){
            long millisSinceStart = _clock->milliseconds() - _startMillis;
            if ( _running ){
                RespawnTimerState newState = computeStateForTimer(_config,millisSinceStart);
                updateState(newState);
                return newState;
            }
            else{
                return RespawnTimerState::IDLE;
            }
        };

    protected:
        void updateState(RespawnTimerState newState){
            if ( newState != _lastState){
                if ( _callback != 0 ){
                    _callback(newState,_id);
                }               
            }  
             _lastState = newState;                     
        };

        Clock* _clock;
        bool _running;
        stateCallback _callback = 0;
        RespawnTimerState _lastState = RespawnTimerState::IDLE;
        int _id = 0;
        long _startMillis = MILLIS_NOT_STARTED;
        TimerConfig _config;
        long _lastUpdateMillis=0;
};
#endif
