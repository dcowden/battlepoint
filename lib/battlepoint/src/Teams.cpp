#include <Arduino.h>
#include "Teams.h"
#include <FastLED.h>

CRGB getTeamColor(Team t){
    if (t == Team::RED){
        return CRGB::Red;
    }
    else if (t == Team::BLU ){
        return CRGB::Blue;
    }
    else if ( t == Team::NOBODY){
        return CRGB::Black;
    }
    else{
        return CRGB::Aqua;
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