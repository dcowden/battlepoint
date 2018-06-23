#include <Game.h>
void print_game_mode_text(char* buffer, GameMode mode){
  switch ( mode ){
    case GameMode::KOTH:
       strcpy(buffer, "KOTH");
       break;
    case GameMode::AD:
       strcpy(buffer,"AD");
       break;
    case GameMode::CP:
       strcpy(buffer, "CP");
       break;
    default:
      strcpy(buffer,"??");
  }
  
}