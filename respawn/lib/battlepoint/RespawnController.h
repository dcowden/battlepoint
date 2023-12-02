#ifndef _INC_RESPAWNCONTROLLER_H
#define _INC_RESPAWNCONTROLLER_H
#include <RespawnTimer.h>
#include <Clock.h>
#include <RespawnSettings.h>
//TODO: allow passing this in
#define NUM_CONTROLLER_TIMERS 5
#define NO_TIMER_AVAILABLE -1

TimerConfig DEFAULT_CONFIG = {
    10000, //durationMillis, overridden currently by each press
    3000, //imminentMillis -- how long before time to go
    3000, //afterFinishMillis-- how long to keep blinking after finish?
    1800 //how long to blink after start, to show which timer we are?
};

typedef enum {
  CONFIGURE =1,
  RUN =2
} OperationMode;

class RespawnController{
    public:
        RespawnController(Clock* clock, RespawnSettings* settings){
            _clock = clock;
            _settings = settings;
            for ( int i=0;i<NUM_CONTROLLER_TIMERS; i++){
                _timers[i] = new RespawnTimer(clock,i);
            }
        };

        void stopTimers(){
            for ( int i=0;i<NUM_CONTROLLER_TIMERS; i++){
                _timers[i]->stop();
            }
        };
        void setCallBackForAllTimers(stateCallback callbackFunction){
            for ( int i=0;i<NUM_CONTROLLER_TIMERS; i++){
                _timers[i]->onStateChange(callbackFunction);
            }            
        }
        void update(){
            for ( int i=0;i<NUM_CONTROLLER_TIMERS; i++){
                _timers[i]->state();
            }            
        }
        RespawnTimer* requestRespawn(RespawnDurationSlot slot ){
            TimerConfig config = DEFAULT_CONFIG;
            config.durationMillis = _settings->durations[slot];
            return requestRespawn(config);
        };

        RespawnTimer* requestRespawn(TimerConfig config){

            int timerIndex = findAvailableTimer();
            if ( timerIndex == NO_TIMER_AVAILABLE ){
                return NULL;
            } 

            RespawnTimer* timer = _timers[timerIndex];
            timer->start(config);
            return timer;
        };

        RespawnTimer* getTimer(int index){
            return _timers[index];
        };

    protected:
        int findAvailableTimer(){
            for ( int i=0;i<NUM_CONTROLLER_TIMERS;i++){
                if ( _timers[i]->isAvailable() ){
                    return i;
                }
            }
            return NO_TIMER_AVAILABLE;
        };
        Clock* _clock;
        RespawnTimer* _timers[NUM_CONTROLLER_TIMERS];
        RespawnSettings* _settings;
};
#endif