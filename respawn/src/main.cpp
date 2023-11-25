#include <Arduino.h>
#include <ArduinoLog.h>
#include "TickTwo.h"
#include <FastLED.h>
#include <settings.h>
#include <pins.h>
#include <RespawnUX.h>
#include <RespawnTimer.h>
#include <OneButton.h>
#include <NonBlockingRtttl.h>

#define BATTLEPOINT_VERSION "1.1.4"
#define RESPAWN_POSITIONS 4

RespawnSettings respawnDurations;

CRGB respawnLeds[RESPAWN_POSITIONS];

RespawnTimer respawnPlayer1Timer;
RespawnTimer respawnPlayer2Timer;
RespawnTimer respawnPlayer3Timer;
RespawnTimer respawnPlayer4Timer;

RespawnUX respawnPlayer1UX;
RespawnUX respawnPlayer2UX;
RespawnUX respawnPlayer3UX;
RespawnUX respawnPlayer4UX;

RespawnContext uiContext;


typedef enum {
  CONFIGURE =1,
  RUN =2
} OperationMode;

OperationMode current_mode = OperationMode::RUN;
long configuredSpawnTimeStartMillis =0;

//(pin, active low, enable pullup)
OneButton shortRespawn(Pins::SHORT_DURATION_BTN,true,true);
OneButton mediumRespawn(Pins::MEDIUM_DURATION_BTN,true,true);
OneButton longRespawn(Pins::LONG_DURATION_BTN,true,true);


void enterConfigurationMode(){
    current_mode = OperationMode::CONFIGURE;
    Log.warningln("Entering config mode: disabling timers");
    contextDisableTimers(&uiContext);
    configuredSpawnTimeStartMillis = millis();
}

void enterRunMode(){
    saveSettings(&respawnDurations);
    current_mode = OperationMode::RUN;
    Log.warningln("Entering Running Mode");  
}

void setupUX(){
  respawnPlayer1UX.timer = &respawnPlayer1Timer;
  respawnPlayer2UX.timer = &respawnPlayer2Timer;
  respawnPlayer3UX.timer = &respawnPlayer3Timer;
  respawnPlayer4UX.timer = &respawnPlayer4Timer;
  uiContext.player1 = &respawnPlayer1UX;
  uiContext.player1 = &respawnPlayer2UX;
  uiContext.player1 = &respawnPlayer3UX;
  uiContext.player1 = &respawnPlayer4UX;
  initContext ( &uiContext,respawnLeds,Pins::SOUND);
 
  FastLED.addLeds<NEOPIXEL, Pins::RESPAWN_LEDS>(respawnLeds, RESPAWN_POSITIONS);
  pinMode(Pins::SOUND, OUTPUT);
}

void handleRespawn(RespawnTimer* timer, long durationMillis, long currentTimeMillis){
    Log.warningln("Starting Timer, Slot %d, Duration=%d", timer->id,durationMillis);
    startTimer(timer,durationMillis,currentTimeMillis);
}

void handleRespawnInput(long durationMillis){
  if ( current_mode == OperationMode::RUN){
    //try to find a respawn slot
    long currentTimeMillis = millis();
    if ( isAvailable(&respawnPlayer1Timer,currentTimeMillis) ){
      handleRespawn(&respawnPlayer1Timer,durationMillis,currentTimeMillis);
    }
    else if ( isAvailable(&respawnPlayer2Timer,currentTimeMillis) ){
      handleRespawn(&respawnPlayer2Timer,durationMillis,currentTimeMillis);
    }
    else if ( isAvailable(&respawnPlayer3Timer,currentTimeMillis) ){
      handleRespawn(&respawnPlayer3Timer,durationMillis,currentTimeMillis);
    }   
    else if ( isAvailable(&respawnPlayer4Timer,currentTimeMillis) ){
      handleRespawn(&respawnPlayer4Timer,durationMillis,currentTimeMillis);
    } 
    else{
      Log.warning("No Respawn Slot Available");
      signalNoSlotsAvailable(Pins::SOUND);
    }     
  }
  else{
    Log.warningln("Click Ignored, another button is long-pressed, so we're in config mode");
  }
  
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
  Log.warningln("Configure Duration");
  enterConfigurationMode();
}

//TODO: how to remove this duplication?
void handleSetupShortDurationLongClickEnd(){
  long configuredDuration = millis() - configuredSpawnTimeStartMillis;
  Log.warningln("Configure Short Duration: %d ms", configuredDuration);
  respawnDurations.shortRespawnMillis = configuredDuration;
  enterRunMode();
}

void handleSetupMediumDurationLongClickEnd(){
  long configuredDuration = millis() - configuredSpawnTimeStartMillis;
  Log.warningln("Configure Medium Duration: %d ms", configuredDuration);
  respawnDurations.mediumRespawnMillis = configuredDuration;
  enterRunMode();
}

void handleSetupLongDurationLongClickEnd(){
  long configuredDuration = millis() - configuredSpawnTimeStartMillis;
  Log.warningln("Configure Long Duration: %d ms", configuredDuration);
  respawnDurations.longRespawnMillis = configuredDuration;
  enterRunMode();
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
  contextUpdate(&uiContext,millis());
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
