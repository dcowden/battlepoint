#include <Arduino.h>
#include <ArduinoLog.h>
#include <FastLED.h>
#include <pins.h>
#include <Clock.h>
#include <OneButton.h>
#include <RespawnSettings.h>
#include <RespawnTimer.h>
#include <RespawnController.h>
#include <RespawnSettingsConfigurator.h>
#include <LedController.h>
#include <NonBlockingRtttl.h>
#include <settings.h>

#define BATTLEPOINT_VERSION "1.1.5"
#define CONFIGURE_BLINK_CONFIRM_MILLIS 100
#define RESPAWN_SOUNDOUT_DELAY_MILLIS 300

const char * SOUND_LIFE1 =  "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * SPAWN_START =  "life:d=8,o=4,b=450:e.5,32p.";
const char * RESPAWN_COUNT =  "rc:d=8,o=4,b=450:e.5";
//const char * DEATH =   "death:d=32,o=4,b=355:8b.,p.,8f.5,p.,4p,8f.5,p.,4f5,16p,4e5,16p,4d5,16p,8c.5,p.,8e.,p.,4p,8e.,p.,8c.,p.";
const char * NO_SLOT = "NoSlot:d=32,o=5,b=100:8g,8c";
const char * READY = "ready:d=4,o=5,b=112:16e4,16e4";


CRGB respawnLeds[NUM_CONTROLLER_TIMERS];
RealClock clock;

LedPixel ledPixels[NUM_CONTROLLER_TIMERS] = {
  LedPixel(&clock,respawnLeds,0,CRGB::Black,CRGB::Black),
  LedPixel(&clock,respawnLeds,1,CRGB::Black,CRGB::Black),
  LedPixel(&clock,respawnLeds,2,CRGB::Black,CRGB::Black),
  LedPixel(&clock,respawnLeds,3,CRGB::Black,CRGB::Black),
  LedPixel(&clock,respawnLeds,4,CRGB::Black,CRGB::Black),
};


PixelColor BLINKING_RED = { 100, CRGB::Red, CRGB::Black };
PixelColor SOLID_GREEN = { 0, CRGB::Green, CRGB::Black };
PixelColor COLOR_CONFIGURE_CONFIRM = { 0, CRGB::Blue, CRGB::Black };
PixelColor COLOR_CONFIGURE_START = { 0, CRGB::MediumPurple, CRGB::Black };
PixelColor SOLID_YELLOW = { 0, CRGB::Yellow, CRGB::Black };
PixelColor SOLID_RED = { 0, CRGB::Red, CRGB::Black };
PixelColor SOLID_BLACK = { 0, CRGB::Black, CRGB::Black };
PixelColor COLOR_INACTIVE = { 0, CRGB::Black, CRGB::Black };
PixelColor BLINKING_GREEN = { 100, CRGB::Green, CRGB::Black };

typedef enum {
  MODE_RUNNING = 0,
  MODE_CONFIGURE =1 
} RunMode;

RunMode currentMode;

RespawnSettings respawnSettings;
RespawnSettingsConfigurator settingsConfigurator(&respawnSettings);
RespawnController respawnController(&clock,&respawnSettings);


//(pin, active low, enable pullup)
OneButton shortRespawn(Pins::SHORT_DURATION_BTN,true,true);
OneButton mediumRespawn(Pins::MEDIUM_DURATION_BTN,true,true);
OneButton longRespawn(Pins::LONG_DURATION_BTN,true,true);

PixelColor getColorForState(RespawnTimerState rs){
  if ( rs == RespawnTimerState::RESPAWN_START){
    return BLINKING_RED;
  }
  if( rs == RespawnTimerState::RESPAWNING){
    return SOLID_RED;
  }
  else if ( rs == RespawnTimerState::IMMINENT){
    return SOLID_YELLOW;
  }  
  else if ( rs == RespawnTimerState::FINISHED){
    return BLINKING_GREEN;
  }
  else{ //idle
    return SOLID_BLACK;
  }
}

const char* getMelodyForState(RespawnTimerState rs){
  if( rs == RespawnTimerState::RESPAWN_START){
    return SPAWN_START;
  }
  else if ( rs == RespawnTimerState::FINISHED){
    return SOUND_LIFE1;
  }
  else if ( rs == RespawnTimerState::IMMINENT){
    return READY;
  }
  else{
    return NULL;
  }
}

//TODO: move these two to a separate class, but i was having trouble
//giving a class an array of object refs. see unused LedGroup
void setSolidColor(PixelColor newColor){
  for ( int i = 0;i<NUM_CONTROLLER_TIMERS;i++){
    ledPixels[i].setPixelColor(newColor);
  }
}

void updatePixels(){
  for ( int i = 0;i<NUM_CONTROLLER_TIMERS;i++){
    ledPixels[i].update();
  }  
}

void updateUX(RespawnTimerState newState, int id){
    if ( currentMode == RunMode::MODE_RUNNING){  
        Log.traceln("New State %d on timer id %d", newState, id);
        const char* song = getMelodyForState(newState);
        ledPixels[id].setPixelColor(getColorForState(newState));
        if ( song != NULL){
          Log.traceln("Play Song, index=%s",song);
            rtttl::begin(Pins::SOUND,song);
        }
    }  
}

void setupUX(){
  PixelColor ic = getColorForState(RespawnTimerState::IDLE);
  setSolidColor(ic);
  respawnController.setCallBackForAllTimers(updateUX);
  FastLED.addLeds<NEOPIXEL, Pins::RESPAWN_LEDS>(respawnLeds, NUM_CONTROLLER_TIMERS);
  pinMode(Pins::SOUND, OUTPUT);
}

void setupSettings(){
  initSettings();
  loadSettings(&respawnSettings);
  Log.traceln(F("Loaded Durations: short=%l, med=%l, long=%l"), respawnSettings.durations[0], respawnSettings.durations[1],respawnSettings.durations[2]);
}

void countRespawns(){
    int rc = respawnController.getRespawnCount();
    Log.warningln("There were %d respawns. Counting them out",rc);
    for ( int i=0;i<rc;i++){
       rtttl::begin(Pins::SOUND,RESPAWN_COUNT);
       rtttl::play();
       delay(RESPAWN_SOUNDOUT_DELAY_MILLIS);
    }
}
void handleRespawnClick(RespawnDurationSlot slot){
    Log.infoln("Request Respawn, button %d",slot);
    if ( currentMode == RunMode::MODE_RUNNING){
      RespawnTimer* t = respawnController.requestRespawn(slot);
      if ( t == NULL ){
        Log.warningln("No Respawn Slots Available!");
        rtttl::begin(Pins::SOUND,NO_SLOT);
      }
    }
    else if ( currentMode == RunMode::MODE_CONFIGURE){
      setSolidColor(COLOR_CONFIGURE_CONFIRM);
      updatePixels();
      settingsConfigurator.increment(1);
      delay(CONFIGURE_BLINK_CONFIRM_MILLIS);
      setSolidColor(COLOR_CONFIGURE_START);
      updatePixels();
      Log.infoln("Increment Count to %d sec",settingsConfigurator.currentCount());
    }    
}

void handleRespawnLongClick(RespawnDurationSlot slot){
    if ( currentMode == RunMode::MODE_RUNNING){
      settingsConfigurator.beginConfiguration(slot);
      respawnController.stopTimers();
      setSolidColor(COLOR_CONFIGURE_START);
      currentMode = RunMode::MODE_CONFIGURE;   
      Log.infoln("Start Configure Mode, button %d",slot);
    }
    else if ( currentMode == RunMode::MODE_CONFIGURE){
      settingsConfigurator.finishConfiguration();
      setSolidColor(getColorForState(RespawnTimerState::IDLE));
      Log.infoln("Finished Configuring");
      saveSettings(&respawnSettings);
      Log.infoln(F("SavedSettings: short=%l, med=%l, long=%l"), respawnSettings.durations[0], respawnSettings.durations[1],respawnSettings.durations[2]);
      currentMode = RunMode::MODE_RUNNING;  
    }
}

void setupInputs(){
  shortRespawn.attachClick([](){ handleRespawnClick(RespawnDurationSlot::SPAWN_DURATION_SHORT); });
  mediumRespawn.attachClick([](){ handleRespawnClick(RespawnDurationSlot::SPAWN_DURATION_MEDIUM); });
  longRespawn.attachClick([](){ handleRespawnClick(RespawnDurationSlot::SPAWN_DURATION_LONG); });

  shortRespawn.attachDoubleClick(countRespawns);
  mediumRespawn.attachDoubleClick(countRespawns);
  longRespawn.attachDoubleClick(countRespawns);

  shortRespawn.attachLongPressStart([](){ handleRespawnLongClick(RespawnDurationSlot::SPAWN_DURATION_SHORT); });
  mediumRespawn.attachLongPressStart([](){ handleRespawnLongClick(RespawnDurationSlot::SPAWN_DURATION_MEDIUM); });
  longRespawn.attachLongPressStart([](){ handleRespawnLongClick(RespawnDurationSlot::SPAWN_DURATION_LONG);  }); 
}

void setup() {
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
  currentMode = RunMode::MODE_RUNNING;   
}

void loop() {
  respawnController.update();
  shortRespawn.tick();
  mediumRespawn.tick();
  longRespawn.tick();
  updatePixels();
  rtttl::play();
  FastLED.show();
}
