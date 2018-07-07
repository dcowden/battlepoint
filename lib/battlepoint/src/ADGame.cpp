#include <Game.h>
#include <Teams.h>


/**
  AD game: RED team starts with ownership.

  If the BLU team captures within the timelimit, blue wins, else red wins

  both time meters have max value of the time limit, and count to zero with time
**/

void ADGame::gameTypeInit(){
     _controlPoint->setRedCaptureEnabled(false);
     _controlPoint->setOwner(Team::RED);
     _timer1Meter->setColors(TeamColor::COLOR_YELLOW, TeamColor::COLOR_BLACK);
     _timer2Meter->setColors(TeamColor::COLOR_YELLOW, TeamColor::COLOR_BLACK);     
}; 

void ADGame::updateDisplay(){
    _timer1Meter->setValue(getRemainingSeconds());
    _timer2Meter->setValue(getRemainingSeconds());
    updateCaptureMeter();
    updateOwnerMeter(); 
};

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

int ADGame::getGameTypeRemainingSeconds(){
    //in this mode, the game always ends after a fixed time.
    //blue wins if the CP is captured. otherwise, red wins
    return _options.timeLimitSeconds - getSecondsElapsed();
};

   