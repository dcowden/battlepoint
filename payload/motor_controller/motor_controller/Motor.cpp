#include "Motor.h"
#include <Arduino.h>

//max output on the drives is actually 400. this prevents spikes
//by avoiding the extra current
#define MIN_MOTOR_COMMAND -220.0
#define MAX_MOTOR_COMMAND 220.0

Motor::Motor ( DualVNH5019MotorShield* _theShield, boolean _isLeft){
	motorShield = _theShield;
	isLeft =  _isLeft;
}

double Motor::getVelocity(){
  return velocity;
}
void Motor::setVelocity(double vel ){
  velocity = vel;
  update();
}
void Motor::update(){
  
   int v = (int)velocity;
   if ( v > MAX_MOTOR_COMMAND ) v = MAX_MOTOR_COMMAND;
   if ( v < MIN_MOTOR_COMMAND ) v = MIN_MOTOR_COMMAND;
   //Serial.print("Setting Speed="); Serial.println(v);
   if ( isLeft ){
      motorShield->setM1Speed(v);       
   }
   else{
      motorShield->setM2Speed(v);       
   }
   if ( motorShield->getM1Fault() || motorShield->getM2Fault() ){
    Serial.print("FAULT!!!");
      velocity = 0;
   }

}
