#ifndef __INC_DEADBANDTIMER_H
#define __INC_DEADBANDTIMER_H
#include <Arduino.h>
class DeadbandTimer{
  public:
    DeadbandTimer();
    boolean isInDeadband(long interval);
    boolean notInDeadband(long interval);
  private:
     long lastEvent;
};
#endif