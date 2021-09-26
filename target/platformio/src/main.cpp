#include <Arduino.h>
#include "TickTwo.h"
#include <U8g2lib.h>
#include <FastLED.h>
#include <target.h>
#include <Clock.h>
#include <LedMeter.h>
#include <game.h>
#include "EncoderMenuDriver.h"
#include "trigger.h"
#include <display.h>
#include <util.h>
#include <pins.h>
#include <math.h>
#include <ArduinoLog.h>
#include <settings.h>
#include <ClickEncoder.h>
#include <menu.h>
#include <menuIO/keyIn.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIO.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>
#include <targetscan.h>
#include <target.h>

#define BATTLEPOINT_VERSION "1.0.1"
#define BP_MENU "BP v1.0.1"
//TODO: organize these into groups
#define MENU_MAX_DEPTH 4
#define OFFSET_X 0
#define OFFSET_Y 0
#define DISPLAY_UPDATE_INTERVAL_MS 100
#define VERTICAL_LED_SIZE 16
#define HORIONTAL_LED_SIZE 12
#define GAME_UPDATE_INTERVAL_MS 20
#define HARDWARE_INFO_UPDATE_INTERVAL_MS 1000

#define TRIGGER_MIN 100
#define TRIGGER_MAX 1000
#define TRIGGER_BIG_STEP_SIZE 100
#define TRIGGER_LITTLE_STEP_SIZE 10

#define HIT_MIN 1000
#define HIT_MAX 25000
#define HIT_BIG_STEP_SIZE 1000
#define HIT_LITTLE_STEP_SIZE 500

#define TIME_LIMIT_MIN 10
#define TIME_LIMIT_MAX 1000
#define TIME_LIMIT_BIG_STEP_SIZE 10
#define TIME_LIMIT_LITTLE_STEP_SIZE 1

#define VICTORY_MARGIN_MIN 0
#define VICTORY_MARGIN_MAX 20
#define VICTORY_MARGIN_BIG_STEP_SIZE 5
#define VICTORY_MARGIN_LITTLE_STEP_SIZE 1

#define VICTORY_HITS_MIN 1
#define VICTORY_HITS_MAX 100
#define VICTORY_HITS_BIG_STEP_SIZE 5
#define VICTORY_HITS_LITTLE_STEP_SIZE 1


#define ENCODER_SERVICE_PRESCALER 5
#define POST_INTERVAL_MS 80
#define TIMER_INTERVAL_MICROSECONDS 100

//two shots from an atlas are 50ms apart, so we need to take samples 
//after a hit for no more than 50ms. 
//This is 40ms at 0.1ms/sample, see timer config
//also note that this can't be more than 500
#define TARGET_NUM_SAMPLES 400
#define TARGET_DEFAULT_TRIGGER_LEVEL 4000

//the different types of games we can play
GameState gameState;
GameSettings gameSettings;
RealClock gameClock = RealClock();


CRGB leftLeds[VERTICAL_LED_SIZE];
CRGB centerLeds[VERTICAL_LED_SIZE];
CRGB rightLeds[VERTICAL_LED_SIZE];
CRGB topLeds[2*HORIONTAL_LED_SIZE];
CRGB bottomLeds[2* HORIONTAL_LED_SIZE];

LedMeter leftTopMeter;
LedMeter leftBottomMeter;
LedMeter rightTopMeter;
LedMeter rightBottomMeter;
LedMeter centerMeter;
LedMeter leftMeter;
LedMeter rightMeter;

MeterSettings meters;
volatile int encoderServicePrescaler = 0;
volatile TargetScanner leftScanner;
volatile TargetScanner rightScanner;

typedef enum {
    PROGRAM_MODE_GAME=0,
    PROGRAM_MODE_MENU = 1,
    PROGRAM_MODE_DIAGNOSTICS =2,
    PROGRAM_MODE_TARGET_TEST
} ProgramMode;

int programMode = ProgramMode::PROGRAM_MODE_MENU;

// ESP32 timer thanks to: http://www.iotsharing.com/2017/06/how-to-use-interrupt-timer-in-arduino-esp32.html
// and: https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *timer = NULL;

HardwareInfo hardwareInfo;

//prototypes
void menuIdleEvent();
void localUpdateGame();
void startSelectedGame();
void updateLEDs();
void startDiagnostics();
void stopGameAndReturnToMenu();
void IRAM_ATTR onTimer();

//from example here: https://github.com/neu-rah/ArduinoMenu/blob/master/examples/ESP32/ClickEncoderTFT/ClickEncoderTFT.ino
ClickEncoder clickEncoder = ClickEncoder(Pins::ENC_DOWN, Pins::ENC_UP, Pins::ENC_BUTTON, 2,true);
//ClickEncoderStream encStream(clickEncoder, 1); 

double getBatteryVoltage(){
  int r = analogRead(Pins::VBATT);

  return (double)r / 1575.0 * 2.0 * 5; //ADC_11db = 1v per 1575 count
}

void updateHardwareInfo(){
    hardwareInfo.vBatt = getBatteryVoltage();
}
int readLeftTarget(){
  return analogRead(Pins::TARGET_LEFT);
}

int readRightTarget(){
  return analogRead(Pins::TARGET_RIGHT);
}

void updateDisplayLocal(){
  if ( programMode == PROGRAM_MODE_GAME || programMode == PROGRAM_MODE_TARGET_TEST){
    updateDisplay(gameState,gameSettings);
    updateMeters(&gameState,&gameSettings,&meters);
    updateLEDs(); 
  }
  else if ( programMode == PROGRAM_MODE_DIAGNOSTICS){
    diagnosticsDisplay(hardwareInfo);
  }

}

void localUpdateGame(){  
  updateGame(&gameState, gameSettings, (Clock*)(&gameClock));
  
  if ( gameSettings.gameType == GameType::GAME_TYPE_TARGET_TEST){
      gameSettings.target.hit_energy_threshold += clickEncoder.getValue()*100;
  }

  if ( gameState.status == GameStatus::GAME_STATUS_ENDED ){
    Log.warning("Game Over!");
    Log.warning("Winner=");
    gameOverDisplay(gameState);
    stopGameAndReturnToMenu();
  }

}

TickTwo updateDisplayTimer(updateDisplayLocal,DISPLAY_UPDATE_INTERVAL_MS);
TickTwo gameUpdateTimer(localUpdateGame, GAME_UPDATE_INTERVAL_MS );
TickTwo diagnosticsDataTimer(updateHardwareInfo, HARDWARE_INFO_UPDATE_INTERVAL_MS);

void setupLEDs(){
  FastLED.addLeds<NEOPIXEL, Pins::LED_TOP>(topLeds, 2* HORIONTAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_BOTTOM>(bottomLeds, 2* HORIONTAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_LEFT_EDGE>(leftLeds, VERTICAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_CENTER_VERTICAL>(centerLeds, VERTICAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_RIGHT_EDGE>(rightLeds, VERTICAL_LED_SIZE);
}

void setupEncoder(){
  clickEncoder.setAccelerationEnabled(true);
  clickEncoder.setDoubleClickEnabled(true); 

  //ESP32 timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_INTERVAL_MICROSECONDS, true); //units are microseconds, 100 = 0.1ms
  timerAlarmEnable(timer);
    
}

void loadSettingsForSelectedGameType(){
  timerAlarmDisable(timer);
  loadSettingSlot(&gameSettings, getSlotForGameType(gameSettings.gameType));
  timerAlarmEnable(timer);
}

void saveSettingsForSelectedGameType(){
  timerAlarmDisable(timer);
  saveSettingSlot(&gameSettings, getSlotForGameType(gameSettings.gameType));
  timerAlarmEnable(timer);
}

void setupTargets(){
  pinMode(Pins::TARGET_LEFT,INPUT);
  pinMode(Pins::TARGET_RIGHT,INPUT);
}




//TODO: move menu stuff to another file somehow
Menu::result loadMostHitsSettings(){
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME;
  loadSettingsForSelectedGameType();
  return Menu::proceed;
}
Menu::result loadFirstToHitsSettings(){  
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_HITS;
  loadSettingsForSelectedGameType();
  return Menu::proceed;
}
Menu::result loadOwnZoneSettings(){
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME;
  loadSettingsForSelectedGameType();
  return Menu::proceed;
}
Menu::result loadCPSettings(){
  gameSettings.gameType = GameType::GAME_TYPE_ATTACK_DEFEND;
  loadSettingsForSelectedGameType();
  return Menu::proceed;
}
Menu::result loadTargetTestSettings(){
  gameSettings.gameType = GameType::GAME_TYPE_TARGET_TEST;
  loadSettingsForSelectedGameType();
  gameSettings.hits.to_win = 16;
  gameSettings.timed.max_duration_seconds=999;
  gameSettings.capture.capture_decay_rate_secs_per_hit = 10;  
  printTargetDataHeaders();

  return Menu::proceed;
}

//FIELD(var.name, title, units, min., max., step size,fine step size, action, events mask, styles)
MENU(mostHitsSubMenu, "MostHits", loadMostHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",VICTORY_MARGIN_MIN,VICTORY_MARGIN_MAX,VICTORY_MARGIN_BIG_STEP_SIZE,VICTORY_MARGIN_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)            ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(firstToHitsSubMenu, "FirstToHits", loadFirstToHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.to_win,"Hits","",VICTORY_HITS_MIN,VICTORY_HITS_MAX,VICTORY_HITS_BIG_STEP_SIZE,VICTORY_HITS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",VICTORY_MARGIN_MIN,VICTORY_MARGIN_MAX,VICTORY_MARGIN_BIG_STEP_SIZE,VICTORY_MARGIN_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)            ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(ozSubMenu, "OwnZone", loadOwnZoneSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.capture.capture_cooldown_seconds,"Capture Cooldown","s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.capture.capture_decay_rate_secs_per_hit,"Capture Decay","/s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.capture.hits_to_capture,"HitsToCapture","",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.ownership_time_seconds,"OwnTimeToWin","s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)        
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(adSubMenu, "Capture", loadCPSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.to_win,"Hits","",VICTORY_HITS_MIN,VICTORY_HITS_MAX,VICTORY_HITS_BIG_STEP_SIZE,VICTORY_HITS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)\
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)        
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
 
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(testTargetMenu, "TargetTest",  loadTargetTestSettings, Menu::enterEvent, Menu::wrapStyle    
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);


MENU(mainMenu, BP_MENU, Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,SUBMENU(mostHitsSubMenu)
  ,SUBMENU(ozSubMenu)
  ,SUBMENU(firstToHitsSubMenu)
  ,SUBMENU(adSubMenu)
  ,SUBMENU(testTargetMenu)
  ,OP("Diagnostics",startDiagnostics,Menu::enterEvent)
);

//{{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
const colorDef<uint8_t> colors[6] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,1}},//cursorColor
  {{0,0},{1,0,0}},//titleColor
};

short charWidth = U8_WIDTH/fontW;
short lineHeight = U8_HEIGHT/fontH;

//define output device
Menu::idx_t serialTops[MENU_MAX_DEPTH] = {0};
constMEM Menu::panel panels[] MEMMODE = {{0, 0, charWidth, lineHeight}};
Menu::navNode* nodes[sizeof(panels) / sizeof(Menu::panel)]; //navNodes to store navigation status
Menu::panelsList pList(panels, nodes, 1); //a list of panels and nodes
Menu::idx_t tops[MENU_MAX_DEPTH] = {0,0}; //store cursor positions for each level

MENU_INPUTS(in);

MENU_OUTPUTS(out,MENU_MAX_DEPTH
  ,U8G2_OUT(oled,colors,fontW,fontH,OFFSET_X,OFFSET_Y,{0,0,charWidth,lineHeight})
  ,SERIAL_OUT(Serial)
);

NAVROOT(nav, mainMenu, MENU_MAX_DEPTH, in, out);

EncoderMenuDriver menuDriver = EncoderMenuDriver(&nav, &clickEncoder);

void setupMeters(){
  meters.leftTop = &leftTopMeter;
  meters.leftBottom = &leftBottomMeter;
  meters.rightTop = &rightTopMeter;
  meters.rightBottom = &rightBottomMeter;
  meters.center = &centerMeter;
  meters.left  = &leftMeter;
  meters.right = &rightMeter;


  initMeter(meters.leftTop,"leftTop",topLeds,0,9);
  initMeter(meters.leftBottom,"leftBottom",bottomLeds,0,2*HORIONTAL_LED_SIZE-1);
  initMeter(meters.rightTop,"rightTop",topLeds,HORIONTAL_LED_SIZE,2*HORIONTAL_LED_SIZE-1);
  initMeter(meters.rightBottom,"rightBottom",bottomLeds,HORIONTAL_LED_SIZE,2*HORIONTAL_LED_SIZE-1);
  initMeter(meters.center,"center",centerLeds,0,VERTICAL_LED_SIZE-1);
  initMeter(meters.left,"left",leftLeds,0,VERTICAL_LED_SIZE-1);
  initMeter(meters.right,"right",rightLeds,0,VERTICAL_LED_SIZE-1);
}

void startSelectedGame(){  

  Log.noticeln("Starting Game. Type= %d", gameSettings.gameType);
  Log.warningln("Target Threshold= %l", gameSettings.target.trigger_threshold);
  saveSettingsForSelectedGameType();

  leftScanner.triggerLevel = gameSettings.target.trigger_threshold;
  rightScanner.triggerLevel = gameSettings.target.trigger_threshold;

  enable(&leftScanner);
  enable(&rightScanner);

  startGame(&gameState, &gameSettings, &gameClock);

  Log.traceln("Meter States:");
  oled.clear();
  nav.idleOn();
  if ( gameSettings.gameType == GameType::GAME_TYPE_TARGET_TEST){
     programMode = PROGRAM_MODE_TARGET_TEST;
  }
  else{
    programMode = PROGRAM_MODE_GAME;
  }
 
}
void startDiagnostics(){
  oled.clear();
  nav.idleOn();
  programMode = PROGRAM_MODE_DIAGNOSTICS;
}

void stopGameAndReturnToMenu(){
  oled.setFont(u8g2_font_7x13_mf); 
  oled.clear();
  nav.idleOff();  
  nav.refresh();
  programMode = PROGRAM_MODE_MENU;
}

Menu::result menuIdleEvent(menuOut &o, idleEvent e) {
  switch (e) {
    case idleStart:{
      Log.notice("Suspending Menu");
      oled.clear(); 
      updateDisplayLocal();
      break;
    } 
    case idling:{
      Log.notice("suspended..."); 
      break;
    } 
    case idleEnd:{
       Log.notice("resuming menu.");

       int buttonState = clickEncoder.getButton();
       if ( buttonState == ClickEncoder::DoubleClicked ){
          stopGameAndReturnToMenu();       
       }       
       break;
    }
  }
  return proceed;
}

void updateLEDs(){
  updateLeds(&meters, gameClock.milliseconds());
  FastLED.show();
}

//TODO: do these belong in meter?
void setMeterValue(LedMeter* meter, int val ){
  meter->val = val;
  updateLedMeter(meter);
}

void setAllMetersToValue(int v ){
  setMeterValue(&leftTopMeter,v);
  setMeterValue(&leftBottomMeter,v);      
  setMeterValue(&leftMeter,v);      
  setMeterValue(&rightTopMeter,v);      
  setMeterValue(&rightBottomMeter,v);      
  setMeterValue(&rightMeter,v);
  setMeterValue(&centerMeter,v);
  FastLED.show();       
}

//TODO: magic numbers: move to constants
void setupTargetScanners(){
  initScanner(&leftScanner, TARGET_NUM_SAMPLES, 10, TARGET_DEFAULT_TRIGGER_LEVEL, &readLeftTarget);
  initScanner(&rightScanner, TARGET_NUM_SAMPLES, 10, TARGET_DEFAULT_TRIGGER_LEVEL, &readRightTarget);
  reset(&leftScanner);
  reset(&rightScanner);
}


void POST(){
   Log.noticeln("POST...");
   for ( int i = 0;i<=DEFAULT_MAX_VAL;i++){
      setAllMetersToValue(i);
      FastLED.delay(POST_INTERVAL_MS);                                
   }
   setAllMetersToValue(0);
   Log.noticeln("POST COMPLETE");
}


void setup() {
  setCpuFrequencyMhz(240);
  hardwareInfo.version = BATTLEPOINT_VERSION;
  Serial.begin(115200);
  Serial.setTimeout(500);
  initSettings();
  Log.begin(LOG_LEVEL_SILENT, &Serial, true);
  Log.warning("Starting...");
  initDisplay();
  displayWelcomeBanner(hardwareInfo.version);
  Log.notice("OLED [OK]");
  setupLEDs();    
  Log.notice("LEDS [OK]");  
  setupTargets();
  Log.notice("TARGETS [OK]");
  analogSetAttenuation(ADC_11db);
  analogReadResolution(12);
  Menu::options->invertFieldKeys = false;
  Log.warningln("Complete.");
  oled.setFont(u8g2_font_7x13_mf);
  
  setupMeters();
  POST();
  setupTargetScanners();  
  setupEncoder();
  setup_target_classifier();
  updateDisplayTimer.start();
  gameUpdateTimer.start();
  diagnosticsDataTimer.start();
  loadTargetTestSettings();
  startSelectedGame();
}

void readTargets(){  

  
  //TODO: how to get rid of this left/rigth dupcliation?
  int current_time_millis = gameClock.milliseconds();

  if ( isReady(&leftScanner)){      
      TargetHitData td = analyze_impact(&leftScanner, gameSettings.target.hit_energy_threshold,false);      
      if ( programMode == PROGRAM_MODE_TARGET_TEST){
        if ( td.hits > 0){
          printTargetData(&td,'L');  
        }
        
      }
      applyLeftHits(&gameState, td, current_time_millis );    
      gameState.lastHit = td;
      Log.warningln("Left Trigger, hits=%d, sampletime = %l",td.hits , leftScanner.sampleTimeMillis );
      enable(&leftScanner);
  }

  if ( isReady(&rightScanner)){
      TargetHitData td = analyze_impact(&rightScanner, gameSettings.target.hit_energy_threshold,false);
      if ( programMode == PROGRAM_MODE_TARGET_TEST){
        if ( td.hits > 0){
          printTargetData(&td,'R');  
        }
      }   
      applyRightHits(&gameState, td, current_time_millis );    
      gameState.lastHit = td;
      enable(&rightScanner);
  } 
}


void loop() {  

  //diagnosticsDataTimer.update();
  

  if ( programMode == PROGRAM_MODE_GAME || programMode == PROGRAM_MODE_TARGET_TEST || programMode == PROGRAM_MODE_DIAGNOSTICS){
      readTargets();
      gameUpdateTimer.update();
      updateDisplayTimer.update();
      
      int b = clickEncoder.getButton();
      if ( b == ClickEncoder::DoubleClicked){
        stopGameAndReturnToMenu();
      }
  }
  else if ( programMode == PROGRAM_MODE_MENU){
      menuDriver.update();
      nav.doInput();
      oled.setFont(u8g2_font_7x13_mf);
      oled.firstPage();
      do nav.doOutput(); while (oled.nextPage() );
  }
  else{
      Serial.println("Unknown Program Mode");
  }
  
}

// ESP32 timer
void IRAM_ATTR onTimer()
{  
  if ( encoderServicePrescaler++ >= ENCODER_SERVICE_PRESCALER ){
    encoderServicePrescaler = 0;
    clickEncoder.service();  
  }
  
  if ( nav.sleepTask){
    long l = millis();
    tick(&leftScanner,l);
    tick(&rightScanner,l);
  }

}