#include <Game.h>
#include <Teams.h>


void KothGame::gameTypeInit(){
     _timer1Meter->setColors(TeamColor::COLOR_RED, TeamColor::COLOR_BLACK);
     _timer2Meter->setColors(TeamColor::COLOR_BLUE, TeamColor::COLOR_BLACK);
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