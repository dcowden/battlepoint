#include "ControlPoint.h"
#include "Teams.h"
#include "Proximity.h"

ControlPoint::ControlPoint() {}

void ControlPoint::init(int secondsToCapture){
   _secondsToCapture = secondsToCapture;
   _enableBluCapture = true;
   _enableRedCapture = true;   
   _capturing = Team::NOBODY;
   _on=Team::NOBODY;
   _owner=Team::NOBODY;
   _captureMillis = 0;
   _contested=false;
   _lastUpdateTime = millis();
}

void ControlPoint::setRedCaptureEnabled(boolean redCapture){
  _enableRedCapture = redCapture;
}

boolean ControlPoint::getRedCaptureEnabled(){
  return _enableRedCapture;
}
boolean ControlPoint::getBluCaptureEnabled(){
  return _enableRedCapture;
}
boolean ControlPoint::isContested(){
  return _contested;
}

void ControlPoint::setBluCaptureEnabled(boolean bluCapture){
  _enableBluCapture = bluCapture;
}    

void ControlPoint::update(Proximity* proximity  ){
  boolean red_on = proximity->isRedClose();
  boolean blu_on = proximity->isBluClose();

  _contested = false;
  boolean is_one_team_on = false;
  

  if ( red_on && ! blu_on ){
    _on = Team::RED;
    is_one_team_on = true;
  }
  else if ( blu_on && ! red_on ){
    _on = Team::BLU;
    is_one_team_on = true;
  }
  else if ( blu_on && red_on ){
    _on = Team::ALL;
    _contested = true;
  }
  else{
    _on = Team::NOBODY;
  }
  //check for start of capture
  //you can start capturing if you are the only one on it, and
  //  (a) nobody owns the point
  //  (b) the other guy owns the point
  if ( _capturing == Team::NOBODY && is_one_team_on ){
    if ( _owner == Team::NOBODY || _on != _owner ){
        if ( ( (_on == Team::RED) && _enableRedCapture  ) || ((_on == Team::BLU) && _enableBluCapture )){
          _capturing = _on;
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
  else if ( _on == Team::NOBODY  ){
    _dec_capture(millisSinceLastUpdate);
  }

  _check_capture();  
  _lastUpdateTime = millis();

}

void ControlPoint::_check_capture(){
  if ( _captureMillis  >= _secondsToCapture*1000 ){
    _owner = _capturing;
    _capturing = Team::NOBODY;
    _captureMillis = 0;
  }
}

void ControlPoint::_dec_capture(long milliSeconds){
    _captureMillis = _captureMillis - milliSeconds;
    if ( _captureMillis <= 0 ){
      _capturing = Team::NOBODY;
      _captureMillis  = 0;
    }
}

void ControlPoint::_inc_capture(long milliSeconds){
  _captureMillis = _captureMillis + milliSeconds;
  long catpureMS = _secondsToCapture*1000;
  if ( _captureMillis > catpureMS ){
      _captureMillis = catpureMS;
  }
}

void ControlPoint::setOwner(Team owner){
  _owner = owner;
}

boolean ControlPoint::isOn(Team t){
  return _on == t || _on == Team::ALL;
};

Team ControlPoint::getCapturing(){
  return _capturing;
};


Team ControlPoint::getOwner(){
  return _owner;
}
boolean ControlPoint::isOwnedBy(Team t){
  return _owner == t;
}

int ControlPoint::getPercentCaptured(){
  return (int)(_captureMillis/ (_secondsToCapture * 10) );
}


