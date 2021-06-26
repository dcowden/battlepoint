#include <Arduino.h>
#include <Wire.h>
#include "I2CAnything.h"
#include "MotorCommands.h"



Motor_Command motorCommand { 0.0, 0.0 , false};


void setup() {
  Wire.begin();

}

void loop() {
  // put your main code here, to run repeatedly:
  for ( int i=0;i<10;i++){
     motorCommand.leftVelocity = (float) 300*i;
     sendSpeedCommand();
     delay(200);
  }
}

void sendSpeedCommand(){
    Wire.beginTransmission (MOTOR_CONTROLLER_ADDRESS );
    I2C_writeAnything (motorCommand);
    Wire.endTransmission ();  
}
