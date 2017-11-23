#include "battlepoint.h"

DeadbandTimer::DeadbandTimer(){
  lastEvent = 0;
}

boolean DeadbandTimer::isInDeadband(long interval){
  return ! notInDeadband(interval);
}

boolean DeadbandTimer::notInDeadband(long interval){
  boolean isout = (( millis() - lastEvent ) > interval) ;
  if ( isout ){
     lastEvent = millis();
  }
  return isout;
}
