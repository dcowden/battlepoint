#include "battlepoint.h"

#define OWNER_METER_MAX 10
#define SWITCH_METER_MAX 10

ControlPoint::ControlPoint(LedMeter* ownerMeter, LedMeter* captureMeter, 
                 EventManager* eventManager,LedMeter* bluSwitch, LedMeter* redSwitch) 
{
    _captureMeter = captureMeter;
    _ownerMeter = ownerMeter;
    _eventManager = eventManager;     
    _bluSwitch = bluSwitch;
    _redSwitch = redSwitch;
}

void ControlPoint::init(int secondsToCapture){
   
   _secondsToCapture = secondsToCapture;
   _enableBluCapture = true;
   _enableRedCapture = true;   
   _capturing = NOBODY;
   _on=NOBODY;
   _owner=NOBODY;
   _value = 0;
   _contested=false;
   _lastUpdateTime = millis();
   _captureMeter->init();
   _ownerMeter->init();
   _ownerMeter->setMaxValue(OWNER_METER_MAX);
   _captureMeter->setMaxValue(_secondsToCapture);
   _bluSwitch->init();
   _redSwitch->init();
   _bluSwitch->setMaxValue(SWITCH_METER_MAX);
   _redSwitch->setMaxValue(SWITCH_METER_MAX);
   _bluSwitch->fgColor(CRGB::Blue);
   _redSwitch->fgColor(CRGB::Red);   
   _bluSwitch->setToMax();
   _redSwitch->setToMax();

   _shouldContestMessage = true;
  update_display();
}

void ControlPoint::setRedCapture(boolean redCapture){
  _enableRedCapture = redCapture;

}

void ControlPoint::setBluCapture(boolean bluCapture){
  _enableBluCapture = bluCapture;
}    

void ControlPoint::update_display(){
  _captureMeter->fgColor(getCapturingColor());  
  _captureMeter->setValue(_value/1000);

  _ownerMeter->fgColor(getOwnerColor());
  _ownerMeter->setToMax();
  
}
void ControlPoint::endGame(uint8_t winner){
  _ownerMeter->fgColor(get_team_color(winner));
  _captureMeter->fgColor(get_team_color(winner));
  _ownerMeter->setToMax();
  _captureMeter->setToMax();
  
}

void ControlPoint::update(Proximity* proximity ){
  update(proximity->isRedClose(),proximity->isBluClose());  
}

void ControlPoint::update(boolean red_on, boolean blu_on ){
  
  _contested = false;
  boolean is_one_team_on = false;
  

  if ( red_on && ! blu_on ){
    _on = RED;
    is_one_team_on = true;
  }
  else if ( blu_on && ! red_on ){
    _on = BLU;
    is_one_team_on = true;
  }
  else if ( blu_on && red_on ){
    _on = BOTH;
    _contested = true;
    if ( _shouldContestMessage ){
      _eventManager->control_point_contested();
      _shouldContestMessage = false;
    }
  }
  else{
    _on = NOBODY;
  }
  //Serial.println("Y");
  if ( ! _contested ){
    _shouldContestMessage = true;
  }
  //check for start of capture
  //you can start capturing if you are the only one on it, and
  //  (a) nobody owns the point
  //  (b) the other guy owns the point
  if ( _capturing == NOBODY && is_one_team_on ){
    if ( _owner == NOBODY || _on != _owner ){
        if ( (_on == RED && _enableRedCapture) || (_on == BLU && _enableBluCapture)){
          _capturing = _on;
          _eventManager->control_point_being_captured(_capturing);
        }
    }
  }
  long millisSinceLastUpdate = (millis() - _lastUpdateTime);
  //check for point capture
  if ( is_one_team_on ){    
    if ( _capturing == _on ){
        _inc_capture(millisSinceLastUpdate);
    }
    else if ( _capturing != _on ){
        _dec_capture(millisSinceLastUpdate);
    }
  }
  // else if ( _on == NOBODY && _owner != NOBODY ){
  else if ( _on == NOBODY  ){
    _dec_capture(millisSinceLastUpdate);
  }

  //check for capture
  _check_capture();
  _debug_state();   
  update_display();
  
  _lastUpdateTime = millis();

}

void ControlPoint::_check_capture(){
  if ( _value >= _secondsToCapture*1000 ){
    _owner = _capturing;
    _capturing = NOBODY;
    _value = 0;
    _eventManager->control_point_captured(_owner);
  }
}

void ControlPoint::_dec_capture(long milliSeconds){
    _value = _value - milliSeconds;
    if ( _value <= 0 ){
      _capturing = NOBODY;
      _value  = 0;
    }
}

void ControlPoint::_inc_capture(long milliSeconds){
  _value = _value + milliSeconds;
  long catpureMS = _secondsToCapture*1000;
  if ( _value > catpureMS ){
      _value = catpureMS;
  }
}

void ControlPoint::setOwner(uint8_t owner){
  _owner = owner;
}

uint8_t ControlPoint::getOn(){return _on;};
uint8_t ControlPoint::getCapturing(){return _capturing;};

void ControlPoint::_debug_state(){
  #ifdef BP_DEBUG  
  char sbuff[30];
  char team[6];
  strcpy(sbuff,"Cap:  ");
  print_team_text(team,_capturing);
  strcat(sbuff,team);
  strcat(sbuff," Own:  ");
  print_team_text(team,_owner);
  strcat(sbuff,team);
  strcat(sbuff," ON:   ");
  print_team_text(team,_on);
  strcat(sbuff,team);
  Serial.print(sbuff);
  Serial.print(F(" V:"));
  Serial.print(_value);
  Serial.print(F("\n"));
  #endif
}


CRGB ControlPoint::getOwnerColor(){
  return get_team_color(_owner);
};

CRGB ControlPoint::getCapturingColor(){
  if ( _contested ){
    return CRGB::Purple;
  }
  else{
    return get_team_color(_capturing);
  }
}

uint8_t ControlPoint::getOwner(){
  return _owner;
}

long ControlPoint::getValue(){
  return _value;
}

boolean ControlPoint::captured(){
  return _owner != NOBODY && (_on == NOBODY || _on == _owner ) && _value <= 0;
}

boolean ControlPoint::capturedBy(uint8_t owner){
  return captured() && _owner == owner;
}

