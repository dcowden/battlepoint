#include <Arduino.h>
#include "Payload.h"
#include "MotorCommands.h"



Payload::Payload(){
    fwd_btn_1=0;
    fwd_btn_2=0;
    fwd_btn_3=0;
    bwd_btn_1=0;
    left_sensor=0;
    right_sensor = 0;  
    lastError=0.0;
    manualDrive=false;
    enabled=false;
    MotorCommand mc { 0.0, 0.0, false};
    lastCommand = mc;


}

int Payload::getNumberForwardButtonsPressed(){
  return num_fwd_pressed ();
}
void Payload::enable(){
  enabled = true;
  lastCommand.enabled = true;
}

void Payload::disable(){
  enabled = false;
  setBothVelocity(0.0);
  lastCommand.enabled = false;     
}
boolean Payload::isDefended(){
  return bwd_btn_1;
}

void Payload::update(){

  if ( !enabled ){
    return;
  }

  fwd_btn_1 = readButton(BTN_FWD_1);
  fwd_btn_2 = readButton(BTN_FWD_2);
  fwd_btn_3 = readButton(BTN_FWD_3);
  bwd_btn_1 = readButton(BTN_BWD);
  left_sensor = readADCPinPeak(WIRE_SENSOR_LEFT,NUM_ADC_SAMPLES);
  right_sensor= readADCPinPeak(WIRE_SENSOR_RIGHT,NUM_ADC_SAMPLES);


  if ( manualDrive){
    computeManualDrive();
  }
  else{
    computeInGameDrive();
  }

}

void Payload::computeManualDrive(){
  if ( fwd_btn_1 ){
    setVelocity(options.fwdSpeed_1,0.0);
  }
  else if ( fwd_btn_2){
    setVelocity(0.0,options.fwdSpeed_1);
  }
  else if ( fwd_btn_3 ){
    setBothVelocity(options.fwdSpeed_1);
  }
  else{
    stopAllMotors();
  }
}

void Payload::computeInGameDrive(){
  int nominalSpeed = computeNominalSpeed();
  boolean shouldDrive = ( nominalSpeed != 0 );
  boolean goingForward = ( nominalSpeed > 0 );

  if ( shouldDrive ){
    int error = left_sensor - right_sensor;
    
    int d_err = error - lastError;

    int correction = (float)error * P_GAIN + (float)d_err * D_GAIN ;
    lastError = error;

    if (goingForward ){
      if ( left_sensor > SENSOR_CLOSE || right_sensor > SENSOR_CLOSE ){
        nominalSpeed = SLOW_SPEED;
      }
      setVelocity(  constrain(nominalSpeed - correction,MIN_SPEED,MAX_SPEED),
                    constrain(nominalSpeed + correction,MIN_SPEED,MAX_SPEED)
      );
    }
    else{   
      //later we want to actually drive backwards.
      //but that will require more sensors
      stopAllMotors();    
    }
  }
  else{
    stopAllMotors();
  }

}

int Payload::readButton(int buttonPin){
  int i = digitalRead(buttonPin);
  return i == 0;
}

int Payload::computeNominalSpeed(){
  int num_fwd = num_fwd_pressed();
  if ( bwd_btn_1 ){
    return 0;
  }
  else{
    if ( num_fwd == 3 ){
      return options.fwdSpeed_3 ;
    }  
    else if ( num_fwd == 2 ){
      return options.fwdSpeed_2;
    }
    else if ( num_fwd == 1 ){
      return options.fwdSpeed_1;
    }
    else{
      return 0;
    }
  }
}

void Payload::setVelocity(float left, float right){
  lastCommand.leftVelocity = left;
  lastCommand.rightVelocity = right;
  lastCommand.enabled = true;
}
void Payload::setBothVelocity(float both){
  setVelocity(both,both);
}

void Payload::stopAllMotors(){
    lastCommand.enabled = false;
    setBothVelocity(0.0);  
}
boolean Payload::isContested(){
  return (num_fwd_pressed() > 0 &&  bwd_btn_1 );
}
boolean Payload::isMoving(){
  return (num_fwd_pressed() > 0 && (! bwd_btn_1));
}
int Payload::num_fwd_pressed ( ){
  return fwd_btn_1 + fwd_btn_2 + fwd_btn_3;
}

int Payload::readADCPinPeak(int pin, int samples ){
  int maxv = 0;
  long total = 0;
  int v = 0;
  for (int i=0;i<samples;i++){
    v = analogRead(pin);
    total += v;
    //if ( v > maxv ) maxv = v;
  }
  return total/samples;
}

Game::Game(Payload* _payload){
  payload = _payload;
  overTime = false;
  running =false;
} 

boolean Game::isOverTime(){
  return overTime;
}

int Game::getSecondsRemaining(){
  if ( running ){
    long endTime = startTime + 1000*options.gameTimeSeconds;
    return (int)(endTime - millis() ) / 1000; 
  }
  else{
    if ( DEBUG ){ Serial.println("WARN: getSecondsRemaining when not running"); };
    return 0;
  }
}

boolean Game::isOver(){
  if ( running ){
    if ( shouldEndGame() ){
      if ( DEBUG ){ Serial.println("Ending Game"); };
      running = false;
      overTime = false;
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return true;
  }
}

boolean Game::shouldEndGame(){
  if ( getSecondsRemaining() <= 0 ){
    if ( payload->isMoving() ){   
      //attackers ONLY possess        
      lastNonPossessionTime = millis();
      return false;
    }
    else if ( payload->isDefended() ){
      //defenders ONLY possess, or contest
      return true;
    }
    else{
      //nobody has possession 
      int non_posession_time = ( millis() - lastNonPossessionTime);
      if ( non_posession_time > ENDGAME_OVERTIME_MAX_NON_POSSESSION_MILLIS){
        return true;
      }
      else{
        return false;
      }
    }
  }
  else{
    return false;  
  }
}

void Game::start(){
  startTime = millis();
  running = true;
}
