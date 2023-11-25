#include <Arduino.h>
#include <ArduinoLog.h>
#include <FastLED.h>
#include <settings.h>
#include <pins.h>
#include <RespawnApp.h>
#include <OneButton.h>
#include <NonBlockingRtttl.h>

#define BATTLEPOINT_VERSION "1.1.4"
#define RESPAWN_POSITIONS 4

RespawnSettings respawnDurations;

CRGB respawnLeds[RESPAWN_POSITIONS];

RespawnApp ux = RespawnApp(respawnLeds,Pins::SOUND);

//(pin, active low, enable pullup)
OneButton shortRespawn(Pins::SHORT_DURATION_BTN,true,true);
OneButton mediumRespawn(Pins::MEDIUM_DURATION_BTN,true,true);
OneButton longRespawn(Pins::LONG_DURATION_BTN,true,true);

void setupUX(){
  FastLED.addLeds<NEOPIXEL, Pins::RESPAWN_LEDS>(respawnLeds, RESPAWN_POSITIONS);
  pinMode(Pins::SOUND, OUTPUT);
  ux.init();
}

void handleRespawnInput(long durationMillis){
    ux.requestRespawn(durationMillis,millis());
}

void handleShortRespawnClick(){
    handleRespawnInput(respawnDurations.shortRespawnMillis);
}
void handleMediumRespawnClick(){
    handleRespawnInput(respawnDurations.mediumRespawnMillis);
} 
void handleLongRespawnClick(){
    handleRespawnInput(respawnDurations.longRespawnMillis);
} 

void handleSetupLongClickStart(){
  ux.startConfigureRespawnTime(millis());
}

//TODO: how to remove this duplication?
void handleSetupShortDurationLongClickEnd(){
  long configuredDuration =ux.getConfiguredDuration(millis());
  Log.warningln("Configure Short Duration: %d ms", configuredDuration);
  respawnDurations.shortRespawnMillis = configuredDuration;
  saveSettings(&respawnDurations);
}

void handleSetupMediumDurationLongClickEnd(){
  long configuredDuration =ux.getConfiguredDuration(millis());
  Log.warningln("Configure Medium Duration: %d ms", configuredDuration);
  respawnDurations.mediumRespawnMillis = configuredDuration;
  saveSettings(&respawnDurations);
}

void handleSetupLongDurationLongClickEnd(){
  long configuredDuration =ux.getConfiguredDuration(millis());
  Log.warningln("Configure Long Duration: %d ms", configuredDuration);
  respawnDurations.longRespawnMillis = configuredDuration;
  saveSettings(&respawnDurations);
}

void setupSettings(){
  initSettings();
  loadSettings(&respawnDurations);
}

void setupInputs(){
  shortRespawn.attachClick(handleShortRespawnClick);
  mediumRespawn.attachClick(handleMediumRespawnClick);
  longRespawn.attachClick(handleLongRespawnClick);

  shortRespawn.attachLongPressStart(handleSetupLongClickStart);
  mediumRespawn.attachLongPressStart(handleSetupLongClickStart);
  longRespawn.attachLongPressStart(handleSetupLongClickStart);  

  shortRespawn.attachLongPressStop(handleSetupShortDurationLongClickEnd);
  mediumRespawn.attachLongPressStop(handleSetupMediumDurationLongClickEnd);
  longRespawn.attachLongPressStop(handleSetupLongDurationLongClickEnd); 
}

void updateUX(){  
  ux.update(millis());
  rtttl::play();
  FastLED.show();
}

void updateInputs(){
  shortRespawn.tick();
  mediumRespawn.tick();
  longRespawn.tick();
}

void setup() {
    setCpuFrequencyMhz(240);
    Serial.begin(115200);
    Serial.setTimeout(500);
    Log.begin(LOG_LEVEL_INFO, &Serial, true);
    Log.warning("Starting...");
    setupSettings();
    Log.noticeln("LOAD SETTINGS [OK]");
    setupInputs();
    Log.noticeln("LOAD INPUTS [OK]");  
    setupUX();
    Log.noticeln("LOAD UX [OK]");    
}

void loop() {
  updateInputs();
  updateUX();
}
