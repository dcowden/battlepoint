#include <Game.h>
#include <Teams.h>
#include <util.h>

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

void CPGame::init(){
     _timer1Meter->setFgColor(CRGB::Red);
     _timer1Meter->setBgColor(CRGB::Black);
     _timer2Meter->setFgColor(CRGB::Blue);
     _timer2Meter->setBgColor(CRGB::Black);
};    