#include <EEPROM.h>
#include <Arduino.h>
#include <ArduinoLog.h>
#include <game.h>

#define BP_CURRENT_SETTINGS_VERSION 203
#define EEPROM_SIZE 320

void initSettings(){
    EEPROM.begin(EEPROM_SIZE);
}

int getSlotAddress(int slot_num){
  return slot_num*sizeof(GameSettings);
}

void saveSettingSlot (GameSettings* gameSettings, int slot_num){    
  int addr = getSlotAddress(slot_num);
  Log.noticeln("Saving Slot %d, addr %d, version= %d",slot_num,addr, gameSettings->BP_VERSION);
  gameSettings->BP_VERSION = BP_CURRENT_SETTINGS_VERSION;
  EEPROM.put(addr, gameSettings);
  EEPROM.commit();
}

void loadSettingSlot(GameSettings* gameSettings,int slot_num){

  GameSettings s;
  int addr = getSlotAddress(slot_num);
  Log.noticeln("Loading Slot %d, addr %d",slot_num,addr);

  EEPROM.get(addr,s);
  Log.noticeln("Loaded WinByHits= %d",s.hits.victory_margin);
  if ( s.BP_VERSION == BP_CURRENT_SETTINGS_VERSION){
    Log.noticeln("Found Valid Settings Version %d in slot %d. Returning Default",s.BP_VERSION, slot_num);
    EEPROM.get(addr,gameSettings);
  }
  else{
    Log.noticeln("Found Invalid Settings Version %d in slot %d. Not Updated",s.BP_VERSION, slot_num);
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
    return GameSettingSlot::SLOT_1;
  }
}

