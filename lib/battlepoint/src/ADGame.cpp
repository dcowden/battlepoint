#include <Game.h>
#include <Teams.h>
#include <util.h>
Team ADGame::checkVictory(){

    if ( _controlPoint->getOwner() == Team::BLU ){
      return Team::BLU;
    }
    if ( getRemainingSeconds() <= 0 ){        
      if ( _controlPoint->getCapturing()  != Team::BLU ){          
        return Team::RED;          
      }
    }
    return Team::NOBODY;
};

boolean ADGame::checkOvertime(){
    if ( getRemainingSeconds() <= 0 ){ 
        if ( _controlPoint->getCapturing()  == Team::BLU ){
            return true;
        }
    }
    return false;
};

int ADGame::getRemainingSeconds(){
    //in this mode, the game always ends after a fixed time.
    //blue wins if the CP is captured. otherwise, red wins
    return _options.timeLimitSeconds - getSecondsElapsed();
};

void ADGame::init(){
     _controlPoint->setRedCaptureEnabled(false);
     _timer1Meter->setFgColor(CRGB::Yellow);
     _timer1Meter->setBgColor(CRGB::Black);
     _timer2Meter->setFgColor(CRGB::Yellow);
     _timer2Meter->setBgColor(CRGB::Black);
};    