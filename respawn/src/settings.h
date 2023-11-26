#include <EEPROM.h>
#include <ArduinoLog.h>
#include <Arduino.h>
#include <state.h>

#define BP_CURRENT_SETTINGS_VERSION 208 //change when any items are added to GameSettings struct
#define EEPROM_SIZE 320
#define MAIN_SLOT 0

void initSettings(){
    //for esp32 only
    //EEPROM.begin(EEPROM_SIZE);
}

int getSlotAddress(int slot_num){
  return slot_num*sizeof(RespawnSettings);
}

void saveSettings (RespawnSettings* respawnSettings){   

  int addr = getSlotAddress(MAIN_SLOT);
  respawnSettings->BP_VERSION = BP_CURRENT_SETTINGS_VERSION;
  RespawnSettings toSave = *respawnSettings;
  for (unsigned int t=0; t<sizeof(toSave); t++){
      EEPROM.write(addr + t, *((char*)&toSave + t));  
  }
  //EEPROM.commit();
  Log.noticeln("Settings Saved.");
}

void loadSettings(RespawnSettings* respawnSettings){

  RespawnSettings s;
  int addr = getSlotAddress(MAIN_SLOT);
 
  if ( EEPROM.read(addr) == BP_CURRENT_SETTINGS_VERSION){
    for (unsigned int t=0; t<sizeof(s); t++){
      *((char*)&s + t) = EEPROM.read(addr + t);      
    }
    *respawnSettings = s;
    Log.noticeln("Settings Loaded.");
  }
}


