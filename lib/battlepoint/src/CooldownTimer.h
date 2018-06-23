#ifndef __INC_DEADBANDTIMER_H
#define __INC_DEADBANDTIMER_H
#include <Arduino.h>
class CooldownTimer{
  public:
    CooldownTimer(long intervalMillis){
      lastEvent = 0;
      _intervalMillis = intervalMillis;
    }

    boolean canRun(){
      long now = millis();
      boolean can_run = (( now - lastEvent ) > _intervalMillis) ;
      if ( can_run){
        lastEvent = now;
      }
      return can_run;
    }
  private:
     long lastEvent;
     long _intervalMillis;

};
#endif