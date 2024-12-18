#include "NFCCards.h"
#include <ArduinoLog.h>

void cardSetDefaults( NFCCard &card){
    card.type = NFCCardType::UNKNOWN;
}

void cardSetDefaults( ClassCard &card){  
  card.big_hit=1;
  card.little_hit=1;
  card.max_hp=1;
  card.player_class = PlayerClass::PC_ANONYMOUS;
  card.invuln_ms=1000;
  card.respawn_ms=1000;
  card.team = TeamChar::T_NONE;    
}
void cardSetDefaults( MedicCard &card){
    card.hp = 1;    
}

void cardSetDefaults( FlagCard &card){
    card.value = 1;
    card.team = TeamChar::T_NONE;
}