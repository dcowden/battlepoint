#include <Game.h>
#include <Teams.h>
#include <util.h>

void KothGame::init(){
    _timer1Meter->setFgColor(CRGB::Red);
    _timer1Meter->setBgColor(CRGB::Black);
    _timer2Meter->setFgColor(CRGB::Blue);
    _timer2Meter->setBgColor(CRGB::Black);
};    

Team KothGame::checkVictory(){    
    if ( checkVictory(Team::BLU) ) return Team::BLU;
    if ( checkVictory(Team::RED) ) return Team::RED;
    return Team::NOBODY;
};

boolean KothGame::checkOvertime(){
    //check for blue overtime
    if ( getRemainingSecondsForTeam(Team::BLU) <= 0 && _controlPoint->getCapturing() == oppositeTeam(Team::BLU) ){
        return true;
    }
    if ( getRemainingSecondsForTeam(Team::RED) <= 0 && _controlPoint->getCapturing() == oppositeTeam(Team::RED) ){
        return true;
    }
    return false;
};

int KothGame::getRemainingSeconds(){
    //in this game mode, time remianing is the time left for the team who owns the point's timer
    if (_controlPoint->isOwnedBy(Team::RED)){
        return getRemainingSecondsForTeam(Team::RED);
    }
    else if (_controlPoint->isOwnedBy(Team::BLU) ){
        return getRemainingSecondsForTeam(Team::BLU);
    }
    else{
        //technically, the game could last forever. but lets say the time left
        //is the time after you start capturing
        return _options.timeLimitSeconds;
    }
};

boolean KothGame::checkVictory(Team t){
    if ( _controlPoint->getOwner() == t && getRemainingSecondsForTeam(t) <= 0 ){
        return true;
    }
    else{
        return false;
    }
};