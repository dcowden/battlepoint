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
#define DEFAULT_SHORT_RESPAWN_MILLIS 5000
#define DEFAULT_MEDIUM_RESPAWN_MILLIS 10000
#define DEFAULT_LONG_RESPAWN_MILLIS 20000

CRGB respawnLeds[RESPAWN_POSITIONS];
const char * SOUND_NO_RESPAWN_SLOTS = "Arkanoid:d=4,o=5,b=140:8g6,16p,16g.6,2a#6,32p,8a6,8g6,8f6,8a6,2g6";

RespawnTimer respawnPlayer1Timer;
RespawnTimer respawnPlayer2Timer;
RespawnTimer respawnPlayer3Timer;
RespawnTimer respawnPlayer4Timer;

RespawnUX respawnPlayer1UX;
RespawnUX respawnPlayer2UX;
RespawnUX respawnPlayer3UX;
RespawnUX respawnPlayer4UX;

OneButton shortRespawn(Pins::SHORT_DURATION_BTN,true);
OneButton mediumRespawn(Pins::MEDIUM_DURATION_BTN,true);
OneButton longRespawn(Pins::LONG_DURATION_BTN,true);

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
      rtttl::begin(Pins::SOUND, SOUND_NO_RESPAWN_SLOTS);
    }     
}



void handleShortRespawnClick(){
    handleRespawnInput(DEFAULT_SHORT_RESPAWN_MILLIS);
}

void handleMediumRespawnClick(){
    handleRespawnInput(DEFAULT_MEDIUM_RESPAWN_MILLIS);
} 
void handleLongRespawnClick(){
    handleRespawnInput(DEFAULT_LONG_RESPAWN_MILLIS);
} 

void setupInputs(){
  // link the button 2 functions.
  // button2.attachClick(click2);
  // button2.attachDoubleClick(doubleclick2);
  // button2.attachLongPressStart(longPressStart2);
  // button2.attachLongPressStop(longPressStop2);
  // button2.attachDuringLongPress(longPress2);
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
    setupInputs();
    Log.noticeln("LOAD INPUTS [OK]");  
    setupUX();
    Log.noticeln("LOAD UX [OK]");    
}

void loop() {
  updateInputs();
  updateUX();
}
