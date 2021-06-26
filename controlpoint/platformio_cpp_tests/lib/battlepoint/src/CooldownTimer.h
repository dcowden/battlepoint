#ifndef __INC_DEADBANDTIMER_H
#define __INC_DEADBANDTIMER_H
#include <Arduino.h>
#include <Clock.h>

#define NO_EVENT -1

class CooldownTimer{
  public:
    CooldownTimer(long intervalMillis, Clock* clock){
      lastEvent = NO_EVENT;
      _intervalMillis = intervalMillis;
      _clock = clock;
    }

    boolean canRun(){
      long now = _clock->milliseconds();
      if ( lastEvent == NO_EVENT ){
        lastEvent = now;
        return true;
      }
      else{
        boolean can_run = (( now - lastEvent ) > _intervalMillis) ;
        if ( can_run){
          lastEvent = now;
        }
        return can_run;        
      }
    }
  private:
     long lastEvent;
     long _intervalMillis;
     Clock* _clock;

};
#endif