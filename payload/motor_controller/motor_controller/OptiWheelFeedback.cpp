#include "OptiWheelFeedback.h"
#include <Arduino.h>
#define STALL_MICROS 100000
#define SMALLEST_REAL_UPDATE_MICROS 200
#define ENCODER_SLOTS 2.0
#define SECONDS_PER_MINUTE 60.0
#define US_PER_MS 1000.0
#define MS_PER_S 1000.0

OptiWheelFeedback::OptiWheelFeedback(){
  prevUpdateMicros = 0;
  prevIsrUpdateMicros = 0;
  currentPulseRate = 0;
  pulseCount = 0;
  hasNewData = false;
}

void OptiWheelFeedback::pulse(){
  if ( (micros() - prevIsrUpdateMicros) > SMALLEST_REAL_UPDATE_MICROS ){
		 pulseCount++;
     //if ( pulseCount > 10 ){
        hasNewData = true;
     //}
     prevIsrUpdateMicros = micros();
  }         
}

boolean OptiWheelFeedback::update(){
  unsigned long currentMicros = micros();  
  unsigned long elapsedMicros = currentMicros - prevUpdateMicros;  
  
  if ( hasNewData ){
    long tmp_rate = elapsedMicros/pulseCount;
    currentPulseRate =  tmp_rate;
    noInterrupts();
      hasNewData = false;
      pulseCount = 0;
      prevUpdateMicros = currentMicros;
    interrupts();    
    return true;
  }
  else{
    if ( elapsedMicros > STALL_MICROS ){
      currentPulseRate = 0.0;
      return true;
    }
  }
  return false;
}

long OptiWheelFeedback::getPulseRate(){
   return currentPulseRate;
}

double  OptiWheelFeedback::getRPM(){
  if ( currentPulseRate > 0 ){
    return  US_PER_MS * MS_PER_S / ENCODER_SLOTS *SECONDS_PER_MINUTE /  (float)currentPulseRate;
  }
  return 0.0;
}
