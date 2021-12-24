#ifndef __INC_TRIGGER_H
#define __INC_TRIGGER_H
#include <Arduino.h>
#include <Clock.h>

/**
 * Allows clients to react exactly once to an event stream, within
 * a given time period.
 * new triggers within the deadband are ignored.
 * 
 **/
class Trigger{
    public:
       Trigger(Clock* _clock, long deadband_millis ){
           clock = _clock;
           deadband_time_millis = deadband_millis;
       };

       void trigger(){
          if ( canTrigger() && (! pending_trigger) ){
              doTrigger();
          }
       }

       //returns true only once, ignore new triggers
       bool isTriggered(){
           if ( pending_trigger ){
               pending_trigger = false;
               return true;
           }
           else{
               return false;
           }
       };

    private:
        bool pending_trigger=false;
        long deadband_time_millis = 0;
        long last_trigger_millis = 0;
        Clock* clock;

        void doTrigger(){
            last_trigger_millis = clock->milliseconds();
            pending_trigger = true;
        };

        bool canTrigger(){            
            long elapsedMillisSinceLast = (clock->milliseconds() - last_trigger_millis);
            return (elapsedMillisSinceLast >= deadband_time_millis);
        };
};

#endif