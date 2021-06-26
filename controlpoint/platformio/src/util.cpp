#include "util.h"


void print_team_text(char* buffer, uint8_t team){

  if ( team == RED ){
    strcpy(buffer,"RED");
  }
  else if ( team == BLU ){
    strcpy(buffer, "BLU");
  }  
  else if ( team == BOTH ){
    strcpy(buffer, "R&B");
  }  
  else if ( team == NOBODY ){
    strcpy(buffer,"---");
  }
 
}

CRGB get_team_color(uint8_t team){
  if ( team == NOBODY ){
    return CRGB::Black;
  }
  else if ( team == BLU ){
    return CRGB::Blue;
  }
  else if ( team == RED ){
    return CRGB::Red;
  }
  else{
    return CRGB::Aqua;
  }  
}

char team_text_char( uint8_t team){
  if ( team == RED ){
    return 'R';
  }
  else if ( team == BLU ){
    return 'B';
  }  
  else if ( team == BOTH ){
    return '+';
  }  
  else if ( team == NOBODY ){
    return '-';
  }
  else{
    return '?'; 
  }
}
void print_game_mode_text(char* buffer, uint8_t mode){
  switch ( mode ){
    case GAME_MODE_KOTH:
       strcpy(buffer, "KOTH");
       break;
    case GAME_MODE_AD:
       strcpy(buffer,"AD");
       break;
    case GAME_MODE_CP:
       strcpy(buffer, "CP");
       break;
    default:
      strcpy(buffer,"??");
  }
  
}
