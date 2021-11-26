#include <EEPROM.h>

#define CURRENT_SETTINGS_VERSION 206
#define EEPROM_SIZE 320

typedef enum {
    SLOT_1 = 1
} SettingSlot;

typedef struct {
    int start_delay_secs;
    long game_secs;
    int VERSION;
} ClockSettings;

void initSettings(){
    EEPROM.begin(EEPROM_SIZE);
}

int getSlotAddress(int slot_num){
  return slot_num*sizeof(ClockSettings);
}

void saveSettingSlot (ClockSettings* clockSettings, int slot_num){   

  int addr = getSlotAddress(slot_num);
  clockSettings->VERSION = CURRENT_SETTINGS_VERSION;
  ClockSettings toSave = *clockSettings;
  Log.noticeln("Saving Slot %d, addr %d, version= %d",slot_num,addr, toSave.VERSION);
  for (unsigned int t=0; t<sizeof(toSave); t++){
      EEPROM.write(addr + t, *((char*)&toSave + t));  
  }
  EEPROM.commit();

}
void loadSettingSlot(ClockSettings* clockSettings,int slot_num){

  ClockSettings s;
  int addr = getSlotAddress(slot_num);
  Log.noticeln("Loading Slot %d, addr %d",slot_num,addr);
  if ( EEPROM.read(addr) == CURRENT_SETTINGS_VERSION){
    for (unsigned int t=0; t<sizeof(s); t++){
      *((char*)&s + t) = EEPROM.read(addr + t);      
    }
    Log.noticeln("Found Valid Settings Version %d in slot %d.",s.VERSION, slot_num);
    *clockSettings = s;
  }
  else{
    Log.warningln("Ingoring Invalid Settings Version %d in slot %d.",s.VERSION, slot_num);
  }

}

void saveSetting (ClockSettings* clockSettings){
   saveSettingSlot(clockSettings, SettingSlot::SLOT_1);
}

void loadSetting(ClockSettings* clockSettings)
{
   loadSettingSlot(clockSettings, SettingSlot::SLOT_1);
}

