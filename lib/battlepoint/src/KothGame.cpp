#include <Game.h>
#include <Teams.h>

/**
  koth game: both teams capture the point as much as possible.
  The first team to hold the point for the timelimt duration wins.

  The game lasts forever if nobody ever captures once.

  The time meters have max value- the time limit, and min value zero
**/


void KothGame::gameTypeInit(){
    _timer1Meter->setMaxValue(_options.timeLimitSeconds);
    _timer2Meter->setMaxValue(_options.timeLimitSeconds);  
    _timer1Meter->setColors(TeamColor::COLOR_BLUE, TeamColor::COLOR_BLACK);
    _timer2Meter->setColors(TeamColor::COLOR_RED, TeamColor::COLOR_BLACK);
};    

void KothGame::updateDisplay(){
    _timer1Meter->setValue(getRemainingSecondsForTeam(Team::BLU));
    _timer2Meter->setValue(getRemainingSecondsForTeam(Team::RED));
    _captureMeter->setValue(_controlPoint->getPercentCaptured());    
    Team owner = _controlPoint->getOwner();
    _ownerMeter->setColors(getTeamColor(owner), TeamColor::COLOR_BLACK);
    if ( owner == Team::NOBODY){
        _ownerMeter->setToMin();
    }
    else{
        _ownerMeter->setToMax();
    }
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

int KothGame::getGameTypeRemainingSeconds(){
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