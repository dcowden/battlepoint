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

void setupSettings(){
  initSettings();
  loadSettings(&respawnDurations);
}

void handleSetupLongClickStart(){
  ux.startConfigureRespawnTime(millis());
}

void handleLongPressEnd(int respawnDurationIndex){
  long configuredDuration =ux.getConfiguredDuration(millis());
  Log.warningln("Configure Duration SLot: %d = %dms", respawnDurationIndex, configuredDuration);
  respawnDurations.respawnDurations[respawnDurationIndex] = configuredDuration;
  saveSettings(&respawnDurations);
}

void setupInputs(){
  shortRespawn.attachClick([](){
      ux.requestRespawn(respawnDurations.respawnDurations[RESPAWN_DURATION_SHORT_INDEX],millis());
  });

  mediumRespawn.attachClick([](){
      ux.requestRespawn(respawnDurations.respawnDurations[RESPAWN_DURATION_MEDIUM_INDEX],millis());
  });

  longRespawn.attachClick([](){
      ux.requestRespawn(respawnDurations.respawnDurations[RESPAWN_DURATION_LONG_INDEX],millis());
  });

  shortRespawn.attachLongPressStart(handleSetupLongClickStart);
  mediumRespawn.attachLongPressStart(handleSetupLongClickStart);
  longRespawn.attachLongPressStart(handleSetupLongClickStart);  

  shortRespawn.attachLongPressStop([](){
      handleLongPressEnd(RESPAWN_DURATION_SHORT_INDEX);
  });

  mediumRespawn.attachLongPressStop([](){
      handleLongPressEnd(RESPAWN_DURATION_MEDIUM_INDEX);
  });

  longRespawn.attachLongPressStop([](){
      handleLongPressEnd(RESPAWN_DURATION_LONG_INDEX);    
  }); 
}

void setup() {
  //setCpuFrequencyMhz(240);
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
  shortRespawn.tick();
  mediumRespawn.tick();
  longRespawn.tick();
  ux.update(millis());
  rtttl::play();
  FastLED.show();
}
