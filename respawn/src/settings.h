#include <EEPROM.h>
#include <Arduino.h>
#include <state.h>

#define BP_CURRENT_SETTINGS_VERSION 208 //change when any items are added to GameSettings struct
#define EEPROM_SIZE 320

void initSettings(){
    EEPROM.begin(EEPROM_SIZE);
}

int getSlotAddress(int slot_num){
  return slot_num*sizeof(RespawnSettings);
}

void saveSettingSlot (RespawnSettings* respawnSettings, int slot_num){   

  int addr = getSlotAddress(slot_num);
  respawnSettings->BP_VERSION = BP_CURRENT_SETTINGS_VERSION;
  RespawnSettings toSave = *respawnSettings;
  for (unsigned int t=0; t<sizeof(toSave); t++){
      EEPROM.write(addr + t, *((char*)&toSave + t));  
  }
  EEPROM.commit();
}

void loadSettingSlot(RespawnSettings* respawnSettings,int slot_num){

  RespawnSettings s;
  int addr = getSlotAddress(slot_num);
 
  if ( EEPROM.read(addr) == BP_CURRENT_SETTINGS_VERSION){
    for (unsigned int t=0; t<sizeof(s); t++){
      *((char*)&s + t) = EEPROM.read(addr + t);      
    }
    *respawnSettings = s;
  }
}


