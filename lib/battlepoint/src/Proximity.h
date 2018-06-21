#ifndef __INC_PROX_H
#define __INC_PROX_H
#include <Arduino.h>
#include "Teams.h"
#define BUTTON_PRESSED 0

class Proximity {
  public:
    virtual boolean isRedClose(){ return false; };
    virtual boolean isBluClose(){ return false; };
    virtual boolean isTeamClose(Team t){ return false; };

};

class ButtonProximity: public Proximity {
  public:

    ButtonProximity( int captureButtonSeconds) : Proximity() {
      _captureButtonSeconds = captureButtonSeconds;
    };

    virtual boolean isRedClose(){
      return _last_btn_red_change_millis > 0 &&
        (millis() - _last_btn_red_change_millis) < (_captureButtonSeconds *1000);
    };

    virtual boolean isBluClose(){
      return _last_btn_blu_change_millis > 0 &&
        (millis() - _last_btn_blu_change_millis) < (_captureButtonSeconds *1000);
    };

    virtual boolean isTeamClose(Team t){
      if ( t == Team::RED){
        return isRedClose();
      }
      else if ( t == Team::BLU){
        return isBluClose();
      }
      else{
        return false;
      }
    };
    void update(boolean redButtonPressed, boolean blueButtonPressed){

      if ( redButtonPressed){
        _last_btn_red_change_millis = millis();
      }

      if ( blueButtonPressed){
        _last_btn_blu_change_millis = millis();
      }
    };

  private:
    long _last_btn_red_change_millis;
    long _last_btn_blu_change_millis;
    int _captureButtonSeconds;
};
#endif