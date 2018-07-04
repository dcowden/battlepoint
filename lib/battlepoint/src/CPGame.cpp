#include <Game.h>
#include <Teams.h>
#include <Clock.h>

void CPGame::gameTypeInit(){
     _timer1Meter->setColors(TeamColor::COLOR_RED, TeamColor::COLOR_BLACK);
     _timer2Meter->setColors(TeamColor::COLOR_BLUE, TeamColor::COLOR_BLACK);
}; 

void CPGame::updateDisplay(){
    _timer1Meter->setValue(getAccumulatedSeconds(Team::BLU));
    _timer2Meter->setValue(getAccumulatedSeconds(Team::RED));
    _captureMeter->setValue(_controlPoint->getPercentCaptured());    

    //TODO: factor out: same as KothGame
    Team owner = _controlPoint->getOwner();
    _ownerMeter->setColors(getTeamColor(owner), TeamColor::COLOR_BLACK);
    if ( owner == Team::NOBODY){
        _ownerMeter->setToMin();
    }
    else{
        _ownerMeter->setToMax();
    }
};


Team CPGame::checkVictory(){
    //this kind of game ends when time is up. the winner
    //is the one with the most time
    
    if ( getRemainingSeconds() <= 0 ){
       int redTime = getAccumulatedSeconds(Team::RED);
       int bluTime = getAccumulatedSeconds(Team::BLU);
       if ( redTime == bluTime){
           return Team::NOBODY;
       }
       else if ( redTime > bluTime){
           return Team::RED;
       }
       else{
           return Team::BLU;
       }
    }
    return Team::NOBODY;
};

boolean CPGame::checkOvertime(){
    //if anyone is on the point, its overtime
    return  _controlPoint->getCapturing() != Team::NOBODY;
};

int CPGame::getRemainingSeconds(){
    //in this mode, the game always ends after a fixed time
    //the team with the most accumulated time wins
    return _options.timeLimitSeconds - getSecondsElapsed();
};

   