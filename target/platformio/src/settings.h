#include <EEPROM.h>
#include <Arduino.h>
#include <ArduinoLog.h>
#include <game.h>

#define BP_CURRENT_SETTINGS_VERSION 206
#define EEPROM_SIZE 320

void initSettings(){
    EEPROM.begin(EEPROM_SIZE);
}

int getSlotAddress(int slot_num){
  return slot_num*sizeof(GameSettings);
}

void saveSettingSlot (GameSettings* gameSettings, int slot_num){   

  int addr = getSlotAddress(slot_num);
  gameSettings->BP_VERSION = BP_CURRENT_SETTINGS_VERSION;
  GameSettings toSave = *gameSettings;
  Log.noticeln("Saving Slot %d, addr %d, version= %d",slot_num,addr, toSave.BP_VERSION);
  for (unsigned int t=0; t<sizeof(toSave); t++){
      EEPROM.write(addr + t, *((char*)&toSave + t));  
  }
  EEPROM.commit();

}

void loadSettingSlot(GameSettings* gameSettings,int slot_num){

  GameSettings s;
  int addr = getSlotAddress(slot_num);
  Log.noticeln("Loading Slot %d, addr %d",slot_num,addr);
  if ( EEPROM.read(addr) == BP_CURRENT_SETTINGS_VERSION){
    for (unsigned int t=0; t<sizeof(s); t++){
      *((char*)&s + t) = EEPROM.read(addr + t);      
    }
    Log.noticeln("Found Valid Settings Version %d in slot %d.",s.BP_VERSION, slot_num);
    *gameSettings = s;
  }
  else{
    Log.warningln("Ingoring Invalid Settings Version %d in slot %d.",s.BP_VERSION, slot_num);
  }

}

GameSettingSlot getSlotForGameType(GameType gameType){
  if ( gameType == GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME) {
    return GameSettingSlot::SLOT_1;
  }
  else if ( gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS) {
    return GameSettingSlot::SLOT_2;
  }
  else if (gameType == GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME){
    return GameSettingSlot::SLOT_3;
  }
  else if (gameType == GameType::GAME_TYPE_ATTACK_DEFEND){
    return GameSettingSlot::SLOT_4;
  }
  else if (gameType == GameType::GAME_TYPE_TARGET_TEST){
    return GameSettingSlot::SLOT_5;
  }  
  else{
    Log.warningln("Warning: Unknown Game Type: %d", gameType);
    return GameSettingSlot::SLOT_1;
  }
}

