#include <Arduino.h>
#include "Teams.h"
#include <FastLED.h>

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
        return TeamColor::RED;
    }
    else if (t == Team::BLU ){
        return TeamColor::BLUE;
    }
    else if ( t == Team::NOBODY){
        return TeamColor::BLACK;
    }
    else{
        return TeamColor::AQUA;
    }
}

char teamTextChar(Team team){
  if ( team == Team::RED ){
    return 'R';
  }
  else if ( team == Team::BLU ){
    return 'B';
  }  
  else if ( team == Team::ALL ){
    return '+';
  }  
  else if ( team == Team::NOBODY ){
    return '-';
  }
  else{
    return '?'; 
  }
}

void printTeamText(char* buffer, Team team){

  if ( team == Team::RED ){
    strcpy(buffer,"RED");
  }
  else if ( team == Team::BLU ){
    strcpy(buffer, "BLU");
  }  
  else if ( team == Team::ALL ){
    strcpy(buffer, "ALL");
  }  
  else if ( team == Team::NOBODY ){
    strcpy(buffer,"---");
  }
 
}