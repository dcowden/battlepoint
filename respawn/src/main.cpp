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


//TODO: i hate this. probably should combine these so we have only 4 not 8
RespawnTimer respawnPlayer1Timer;
RespawnTimer respawnPlayer2Timer;
RespawnTimer respawnPlayer3Timer;
RespawnTimer respawnPlayer4Timer;

RespawnUX respawnPlayer1UX;
RespawnUX respawnPlayer2UX;
RespawnUX respawnPlayer3UX;
RespawnUX respawnPlayer4UX;

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


void disableAllTimers(){
    disableTimer(&respawnPlayer1Timer);
    disableTimer(&respawnPlayer2Timer);
    disableTimer(&respawnPlayer3Timer);
    disableTimer(&respawnPlayer4Timer);
}
void enterConfigurationMode(){
    current_mode = OperationMode::CONFIGURE;
    Log.warningln("Entering config mode: disabling timers");
    disableAllTimers();
    configuredSpawnTimeStartMillis = millis();
}

void enterRunMode(){
    saveSettings(&respawnDurations);
    current_mode = OperationMode::RUN;
    Log.warningln("Entering Running Mode");  
}

void setupTimers(){
  loadSettings(&respawnDurations);  
  disableAllTimers();
}

void setupUX(){
  initRespawnUX(&respawnPlayer1UX,respawnLeds,0, Pins::SOUND);
  initRespawnUX(&respawnPlayer2UX,respawnLeds,1, Pins::SOUND);
  initRespawnUX(&respawnPlayer3UX,respawnLeds,2, Pins::SOUND);
  initRespawnUX(&respawnPlayer4UX,respawnLeds,3, Pins::SOUND);
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

void handleSetupShortDurationLongClickStart(){
  Log.warningln("Configure Short Duration");
  enterConfigurationMode();
}

void handleSetupMediumDurationLongClickStart(){
  Log.warningln("Configure Medium Duration");
  enterConfigurationMode();
}

void handleSetupLongDurationLongClickStart(){
  Log.warningln("Configure Long Duration");
  enterConfigurationMode();
}

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

void setupInputs(){
  shortRespawn.attachClick(handleShortRespawnClick);
  mediumRespawn.attachClick(handleMediumRespawnClick);
  longRespawn.attachClick(handleLongRespawnClick);

  shortRespawn.attachLongPressStart(handleSetupShortDurationLongClickStart);
  mediumRespawn.attachLongPressStart(handleSetupMediumDurationLongClickStart);
  longRespawn.attachLongPressStart(handleSetupLongDurationLongClickStart);  

  shortRespawn.attachLongPressStop(handleSetupShortDurationLongClickEnd);
  mediumRespawn.attachLongPressStop(handleSetupMediumDurationLongClickEnd);
  longRespawn.attachLongPressStop(handleSetupLongDurationLongClickEnd); 
}

void updateUX(){
  long current_time_millis = millis();
  
  updateRespawnUX(&respawnPlayer1UX,computeTimerState(&respawnPlayer1Timer,current_time_millis),current_time_millis);
  updateRespawnUX(&respawnPlayer2UX,computeTimerState(&respawnPlayer2Timer,current_time_millis),current_time_millis);
  updateRespawnUX(&respawnPlayer3UX,computeTimerState(&respawnPlayer3Timer,current_time_millis),current_time_millis);
  updateRespawnUX(&respawnPlayer4UX,computeTimerState(&respawnPlayer4Timer,current_time_millis),current_time_millis);
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
    initSettings();
    Log.noticeln("LOAD SETTINGS [OK]");
    setupTimers();
    Log.noticeln("LOAD TIMERS [OK]");
    setupInputs();
    Log.noticeln("LOAD INPUTS [OK]");  
    setupUX();
    Log.noticeln("LOAD UX [OK]");    
}

void loop() {
  updateInputs();
  updateUX();
}
