#include <Arduino.h>
#include <Wire.h>
#include <PID_v1.h>
#include "OptiWheelFeedback.h"
#include "Motor.h"
#include "MotorWithFeedback.h"
#include <timer.h>
#include "I2CAnything.h"
#include "MotorCommands.h"
#include "DualVNH5019MotorShield.h"

#define M1INA 5 //default is 2
#define M1INB 4
#define M1EN 6
#define M2INA 7
#define M2INB 8
#define M1PWM 9
#define M2PWM 10
#define M2EN 12
#define M1CS A0
#define M2CS A1

#define LEFT_MOTOR_ENCODER_PIN 3
#define RIGHT_MOTOR_ENCODER_PIN 2

#define DEBUG false
#define DEBUG_INTERVAL_MS 1000
#define MAXIMUM_SAFE_COMMAND_GAP_MS 500 //disable if you have not heard from sender in this time

auto timer = timer_create_default(); // create a timer with default settings
double KP=0.06;
double KI=0.2; //0.08 
double KD=0.0;


volatile Motor_Command motorCommand {0.0, 0.0,false};
volatile boolean haveNewMotorCommand= false;

volatile float i2c_leftVelocity;
volatile float i2c_rightVelocity;
volatile byte i2c_enable;
volatile long safety_last_command_time=0;
volatile byte safety_enabled = false;

OptiWheelFeedback leftEncoder;
OptiWheelFeedback rightEncoder;

DualVNH5019MotorShield motors(M1INA, M1INB, M1PWM, M1EN, M1CS, M2INA, M2INB, M2PWM, M2EN, M2CS);

Motor leftMotor(&motors, true);
Motor rightMotor(&motors,false);

MotorWithFeedback left ( &leftEncoder , &leftMotor,KP,KI,KD );
MotorWithFeedback right ( &rightEncoder,  &rightMotor,KP,KI,KD );


void leftEncoderISR() {
    leftEncoder.pulse();
}

void rightEncoderISR() {
    rightEncoder.pulse();
}

void receiveEvent(int howMany){
  
  if(howMany >= (sizeof(i2c_leftVelocity) + sizeof(i2c_rightVelocity) + sizeof(i2c_enable))){
      I2C_readAnything (i2c_leftVelocity);
      I2C_readAnything (i2c_rightVelocity);
      I2C_readAnything (i2c_enable);
      haveNewMotorCommand = true;
  }
}

void setupMotorPins(){
  attachInterrupt(digitalPinToInterrupt(LEFT_MOTOR_ENCODER_PIN), leftEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_MOTOR_ENCODER_PIN), rightEncoderISR, RISING);      
}

void setup() 
{
    
    Wire.begin(MOTOR_CONTROLLER_ADDRESS );
    Wire.setClock(100000);
    Serial.begin(115200);
    Wire.onReceive(receiveEvent);
    motors.init();
    setupMotorPins();

    if ( DEBUG ){
      timer.every(DEBUG_INTERVAL_MS, showDebugData);
    }
    //debugSpeeds();
}
void debugSpeeds(){
    left.setTargetVelocity(300);
    right.setTargetVelocity(300);
    left.setEnabled(1);
    right.setEnabled(1);  
}

void safetyCheck(){
  long timeSinceLastCommand = (millis() - safety_last_command_time);
  if ( timeSinceLastCommand > MAXIMUM_SAFE_COMMAND_GAP_MS ){
    disableDrive();  
  }
}

void disableDrive(){
  Serial.println("WARNING: NO COMMAND. DISABLED");
  left.setTargetVelocity(0);
  right.setTargetVelocity(0);
  left.setEnabled(0);
  right.setEnabled(0);
  
}
void loop()
{   
    safetyCheck();
    if ( haveNewMotorCommand ){      
      left.setTargetVelocity(i2c_leftVelocity);
      right.setTargetVelocity(i2c_rightVelocity);    
      left.setEnabled((int)i2c_enable);
      right.setEnabled((int)i2c_enable);  
      haveNewMotorCommand = false;
      safety_last_command_time = millis();
    }
    
    left.update();
    right.update();
    timer.tick();
}

void showDebugData(void *) {
  left.debug('L');
  right.debug('R');
}
