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

void ADGame::gameTypeInit(){
     _controlPoint->setRedCaptureEnabled(false);
     _timer1Meter->setColors(TeamColor::COLOR_YELLOW, TeamColor::COLOR_BLACK);
     _timer2Meter->setColors(TeamColor::COLOR_YELLOW, TeamColor::COLOR_BLACK);     
};    