#include "Teams.h"

bool isHumanTeam(Team t){
    return t == Team::RED || t == Team::BLU;
}

Team oppositeTeam(Team t){
  if ( t == Team::RED){
    return Team::BLU;
  }
  else if ( t == Team::BLU){
    return Team::RED;
  }
  else{
    return Team::NOBODY;
  }
}

TeamColor getTeamColor(Team t){
    if (t == Team::RED){
        return TeamColor::COLOR_RED;
    }
    else if (t == Team::BLU ){
        return TeamColor::COLOR_BLUE;
    }
    else if ( t == Team::NOBODY){
        return TeamColor::COLOR_BLACK;
    }
    else{
        return TeamColor::COLOR_AQUA;
    }
}

const char* teamTextChar(Team team){
  if ( team == Team::RED ){
    return "RED";
  }
  else if ( team == Team::BLU ){
    return "BLU";
  }  
  else if ( team == Team::ALL ){
    return "ALL";
  }  
  else if ( team == Team::NOBODY ){
    return "NONE";
  }
  else{
    return "?"; 
  }
}