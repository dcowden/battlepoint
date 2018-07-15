#include <EEPromSettings.h>

#define DEFAULT_CAPTURE_SECONDS 20
#define DEFAULT_GAME_TIME 60
#define DEFAULT_BUTTON_SECONDS 5
#define DEFAULT_START_DELAY 5
#define DEFAULT_VOLUME 10

#define DEFAULT_BRIGHTNESS 50
#define BP_VERSION 241
#define CONFIG_START 0


SettingsData getDefaultSettings(){

  GameOptions gameOptions = {
      GameMode::KOTH,
      DEFAULT_CAPTURE_SECONDS,
      DEFAULT_BUTTON_SECONDS,
      DEFAULT_START_DELAY,DEFAULT_GAME_TIME
  };
  SettingsData data = {
     //overall options
     BP_VERSION,
     DEFAULT_VOLUME,
     DEFAULT_BRIGHTNESS,
     gameOptions
     
  };
  return data;
}

SettingsData loadSettings(){

  //TODO: replace this with ESP32 library
  //the below worked on Arduino

  //if ( EEPROM.read(CONFIG_START) == BP_VERSION ){
  //  for (unsigned int t=0; t<sizeof(data); t++)
  //    *((char*)&data + t) = EEPROM.read(CONFIG_START + t);    
  //}
  return getDefaultSettings();
}

void saveSettings(SettingsData data ){

   //TODO: replace with working version

   // for (unsigned int t=0; t<sizeof(data); t++)
   //     EEPROM.write(CONFIG_START + t, *((char*)&data + t));
}