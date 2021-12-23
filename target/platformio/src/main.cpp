#include <Arduino.h>
#include "TickTwo.h"
#include <U8g2lib.h>
#include <FastLED.h>
#include <target.h>
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
#include <Teams.h>
#include <sound.h>

#define BATTLEPOINT_VERSION "1.1.1"
#define BP_MENU "BP v1.1.1"
//TODO: organize these into groups
#define MENU_MAX_DEPTH 4
#define OFFSET_X 0
#define OFFSET_Y 0
#define DISPLAY_UPDATE_INTERVAL_MS 100
#define VERTICAL_LED_SIZE 16
#define HORIONTAL_LED_SIZE 12
#define GAME_UPDATE_INTERVAL_MS 200
#define HARDWARE_INFO_UPDATE_INTERVAL_MS 1000

#define TRIGGER_MIN 100
#define TRIGGER_MAX 3000
#define TRIGGER_BIG_STEP_SIZE 500
#define TRIGGER_LITTLE_STEP_SIZE 100

#define HIT_MIN 1
#define HIT_MAX 10
#define HIT_BIG_STEP_SIZE 1
#define HIT_LITTLE_STEP_SIZE 1

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

#define DURATION_SECS_MIN 1
#define DURATION_SECS_MAX 500
#define DURATION_SECS_BIG_STEP_SIZE 10
#define DURATION_SECS_LITTLE_STEP_SIZE 1


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
void refreshDisplay();
void stopGameAndReturnToMenu();
void victoryDance(Team winner);
void captureDance(Team capture);

/*
void handle_game_capture(Team t);
void handle_game_ended(Team t);
void handle_game_cancel();
void handle_game_overtime();
void handle_game_started(GameStatus status);
void handle_game_contested();
void handle_game_remainingsecs(int remaining_secs, GameStatus status);
*/
void IRAM_ATTR onTimer();

//from example here: https://github.com/neu-rah/ArduinoMenu/blob/master/examples/ESP32/ClickEncoderTFT/ClickEncoderTFT.ino
ClickEncoder clickEncoder = ClickEncoder(Pins::ENC_DOWN, Pins::ENC_UP, Pins::ENC_BUTTON, 2,true);


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
  long current_time_millis = millis();
  updateGame(&gameState, gameSettings, current_time_millis);

  sound_gametime_update(gameState.time.remaining_secs,current_time_millis);

  if ( gameSettings.gameType == GameType::GAME_TYPE_TARGET_TEST){
      gameSettings.target.hit_energy_threshold += clickEncoder.getValue()*100;
  }

  if ( gameState.status == GameStatus::GAME_STATUS_ENDED ){
    Log.warning("Game Over!");    
    if ( gameState.result.winner == Team::RED ||  gameState.result.winner == Team::BLU){
      sound_play(SND_SOUNDS_0023_ANNOUNCER_VICTORY,current_time_millis);
    }
    else{
      sound_play(SND_SOUNDS_0018_ANNOUNCER_SD_MONKEYNAUT_END_CRASH02,current_time_millis);
    }
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
  printTargetDataHeaders();
  return Menu::proceed;
}

void handle_game_capture(Team t){
  if ( t != Team::NOBODY){
    captureDance(t);
    sound_play(SND_SOUNDS_0025_ANNOUNCER_WE_CAPTURED_CONTROL,millis());
  }
}

void handle_game_ended(Team winner){
    if ( winner == Team::RED ||  winner == Team::BLU){
      sound_play(SND_SOUNDS_0023_ANNOUNCER_VICTORY,millis());
      victoryDance(winner);
    }
    else{
      sound_play(SND_SOUNDS_0018_ANNOUNCER_SD_MONKEYNAUT_END_CRASH02,millis());
    }
}

void handle_game_cancel(){
  sound_play(SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10,millis() );
}

void handle_game_overtime(){
  sound_play(SND_SOUNDS_0016_ANNOUNCER_OVERTIME,millis() );
}

void handle_game_started(GameStatus status){
  if ( status == GAME_STATUS_PREGAME){
    sound_play(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED,millis() );
  }
  else if ( status == GAME_STATUS_RUNNING){
    sound_play(SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4,millis() );
  }
}
void handle_game_contested(){
  sound_play(SND_SOUNDS_0002_ANNOUNCER_ALERT_CENTER_CONTROL_BEING_CONTESTED,millis() );
}

void handle_game_remainingsecs(int secs_remaining, GameStatus status){
  sound_gametime_update(secs_remaining, millis() );
}

void setupEventHandlers(){
  gamestate_init(&gameState);
  gameState.eventHandler.CapturedHandler = handle_game_capture;
  gameState.eventHandler.ContestedHandler = handle_game_contested;
  gameState.eventHandler.EndedHandler = handle_game_ended;
  gameState.eventHandler.OvertimeHandler =  handle_game_overtime;
  gameState.eventHandler.StartedHandler = handle_game_started;
  gameState.eventHandler.CancelledHandler = handle_game_cancel;
  gameState.eventHandler.RemainingSecsHandler = handle_game_remainingsecs;
} 

//FIELD(var.name, title, units, min., max., step size,fine step size, action, events mask, styles)
MENU(mostHitsSubMenu, "MostHits", loadMostHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",VICTORY_MARGIN_MIN,VICTORY_MARGIN_MAX,VICTORY_MARGIN_BIG_STEP_SIZE,VICTORY_MARGIN_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.hits.to_win,"HitsToWin","",1,100,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)                
    ,FIELD(gameSettings.timed.max_overtime_seconds,"Overtime","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.countdown_start_seconds,"StartDelay","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)        
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(firstToHitsSubMenu, "FirstToHits", loadFirstToHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",VICTORY_MARGIN_MIN,VICTORY_MARGIN_MAX,VICTORY_MARGIN_BIG_STEP_SIZE,VICTORY_MARGIN_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.hits.to_win,"HitsToWin","",VICTORY_HITS_MIN,VICTORY_HITS_MAX,VICTORY_HITS_BIG_STEP_SIZE,VICTORY_HITS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)                
    ,FIELD(gameSettings.timed.max_overtime_seconds,"Overtime","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.countdown_start_seconds,"StartDelay","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)       
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(ozSubMenu, "OwnZone", loadOwnZoneSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.capture.capture_cooldown_seconds,"Cap. Cooldown","s",1,30,5,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.capture.capture_decay_rate_secs_per_hit,"Hit Decay","s",1,100,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.capture.hits_to_capture,"HitsToCapture","",1,500,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.ownership_time_seconds,"OwnTimeToWin","s",1,200,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)                
    ,FIELD(gameSettings.timed.max_overtime_seconds,"Overtime","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.countdown_start_seconds,"StartDelay","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)   
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(adSubMenu, "Capture", loadCPSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.capture.hits_to_capture,"HitsToCapture","",1,500,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.hits.victory_margin,"VictMargin","",VICTORY_MARGIN_MIN,VICTORY_MARGIN_MAX,VICTORY_MARGIN_BIG_STEP_SIZE,VICTORY_MARGIN_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",TIME_LIMIT_MIN,TIME_LIMIT_MAX,TIME_LIMIT_BIG_STEP_SIZE,TIME_LIMIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)                
    ,FIELD(gameSettings.timed.max_overtime_seconds,"Overtime","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.countdown_start_seconds,"StartDelay","s",DURATION_SECS_MIN,DURATION_SECS_MAX,DURATION_SECS_BIG_STEP_SIZE,DURATION_SECS_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)      
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.capture.capture_decay_rate_secs_per_hit,"Hit Decay","s",1,100,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)  
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(testTargetMenu, "TugOWar",  loadTargetTestSettings, Menu::enterEvent, Menu::wrapStyle   
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",VICTORY_MARGIN_MIN,VICTORY_MARGIN_MAX,VICTORY_MARGIN_BIG_STEP_SIZE,VICTORY_MARGIN_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",HIT_MIN,HIT_MAX,HIT_BIG_STEP_SIZE,HIT_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",TRIGGER_MIN,TRIGGER_MAX,TRIGGER_BIG_STEP_SIZE,TRIGGER_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.countdown_start_seconds,"StartDelay","s",1,120,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
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


  initMeter(meters.leftTop,"leftTop",topLeds,0,HORIONTAL_LED_SIZE-1);
  initMeter(meters.rightTop,"rightTop",topLeds,HORIONTAL_LED_SIZE,2*HORIONTAL_LED_SIZE-1);

  initMeter(meters.leftBottom,"leftBottom",bottomLeds,0,HORIONTAL_LED_SIZE-1);
  initMeter(meters.rightBottom,"rightBottom",bottomLeds,HORIONTAL_LED_SIZE,2*HORIONTAL_LED_SIZE-1);

  initMeter(meters.center,"center",centerLeds,0,VERTICAL_LED_SIZE-1);
  initMeter(meters.left,"left",leftLeds,0,VERTICAL_LED_SIZE-1);
  initMeter(meters.right,"right",rightLeds,0,VERTICAL_LED_SIZE-1);
}

void startSelectedGame(){  
  long current_time_millis = millis();
  Log.noticeln("Starting Game. Type= %d", gameSettings.gameType);
  Log.warningln("Target Threshold= %l", gameSettings.target.trigger_threshold);
  saveSettingsForSelectedGameType();

  leftScanner.triggerLevel = gameSettings.target.trigger_threshold;
  rightScanner.triggerLevel = gameSettings.target.trigger_threshold;

  enable(&leftScanner);
  enable(&rightScanner);
  startGame(&gameState, &gameSettings,current_time_millis);
  sound_play_once_in_game(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED,current_time_millis);
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
  refreshDisplay();
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


void setMeterValue(LedMeter* meter, int val ){
  meter->val = val;
  updateLedMeter(meter);
  FastLED.show();
}

CRGB getLedColorForTeam(TeamColor teamColor){
  if ( teamColor == TeamColor::COLOR_BLUE){
    return CRGB::Blue;
  }
  else if ( teamColor == TeamColor::COLOR_RED){
    return CRGB::Red;
  }
  else{
    return CRGB::Yellow;
  }
}


void setAllMetersToValue(int v ){
  setMeterValue(&leftTopMeter,v);
  setMeterValue(&leftBottomMeter,v);      
  setMeterValue(&leftMeter,v);      
  setMeterValue(&rightTopMeter,v);      
  setMeterValue(&rightBottomMeter,v);      
  setMeterValue(&rightMeter,v);
  setMeterValue(&centerMeter,v);
}
void setAllMetersToMax(){
  //TODO: should be a meter method
  setMeterValue(&leftTopMeter,leftTopMeter.max_val);
  setMeterValue(&leftBottomMeter,leftBottomMeter.max_val);      
  setMeterValue(&leftMeter,leftMeter.max_val);      
  setMeterValue(&rightTopMeter,rightTopMeter.max_val);      
  setMeterValue(&rightBottomMeter,rightBottomMeter.max_val);      
  setMeterValue(&rightMeter,rightMeter.max_val);
  setMeterValue(&centerMeter,centerMeter.max_val);     
}

void setAllMeterColors(CRGB fgColor, CRGB bgColor){
  leftTopMeter.fgColor = fgColor;
  leftBottomMeter.fgColor = fgColor;
  leftMeter.fgColor = fgColor;
  rightTopMeter.fgColor = fgColor;
  rightBottomMeter.fgColor = fgColor;
  rightMeter.fgColor = fgColor;
  centerMeter.fgColor = fgColor;

  leftTopMeter.bgColor = bgColor;
  leftBottomMeter.bgColor = bgColor;
  leftMeter.bgColor = bgColor;
  rightTopMeter.bgColor = bgColor;
  rightBottomMeter.bgColor = bgColor;
  rightMeter.bgColor = bgColor;
  centerMeter.bgColor = bgColor;

}
void captureDance( Team capturing){
    const int NUM_FLASHES=10;
    const int DELAY_MS=50;  
    TeamColor tc = getTeamColor(capturing);
    CRGB winnerColor = getLedColorForTeam(tc);
    centerMeter.fgColor = winnerColor;
    centerMeter.bgColor = CRGB::Black;
    for ( int i =0;i<NUM_FLASHES;i++){
      setMeterValue(&centerMeter,0);
      delay(DELAY_MS);
      setMeterValue(&centerMeter,centerMeter.max_val);
      delay(DELAY_MS);
    }  
}

void victoryDance( Team winner ){
  //todo: write to be time based, not number of flash based
  const int NUM_FLASHES=40;

  //TOODO: could simplfiy using LED meter built-in flashing
  //but i like having the control here to do other stuff too
  //really need ability to apply a function to all the meters!
  const int DELAY_MS=100;
  TeamColor tc = getTeamColor(winner);
  CRGB winnerColor = getLedColorForTeam(tc);

  //special, here we set the color of all the meters!
  setAllMeterColors(winnerColor, CRGB::Black);

  for (int i=0;i<NUM_FLASHES;i++){
    setAllMetersToValue(0);
    FastLED.delay(DELAY_MS);  
    setAllMetersToMax();
    FastLED.delay(DELAY_MS);  
  }
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
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
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
  setupEventHandlers();
  setupMeters();
  POST();
  setupTargetScanners();  
  setupEncoder();
  setup_target_classifier();
  updateDisplayTimer.start();
  gameUpdateTimer.start();
  diagnosticsDataTimer.start();
  loadTargetTestSettings();
  sound_init_for_testing();
  refreshDisplay();
}

void readTargets(){  

  //TODO: how to get rid of this left/rigth dupcliation?
  int current_time_millis = gameClock.milliseconds();

  if ( isReady(&leftScanner)){    
      TargetHitData td = analyze_impact(&leftScanner,(int)gameSettings.target.hit_energy_threshold, false);      
      if ( programMode == PROGRAM_MODE_TARGET_TEST){
        if ( td.hits > 0){
          printTargetData(&td,'L');  
        }
        
      }
      applyLeftHits(&gameState, &gameSettings, td, current_time_millis );    
      gameState.lastHit = td;
      Log.warningln("Left Trigger, hits=%d, sampletime = %l",td.hits , leftScanner.sampleTimeMillis );
      enable(&leftScanner);
  }

  if ( isReady(&rightScanner)){
      TargetHitData td = analyze_impact(&rightScanner,(int)gameSettings.target.hit_energy_threshold,false);
      if ( programMode == PROGRAM_MODE_TARGET_TEST){
        if ( td.hits > 0){
          printTargetData(&td,'R');  
        }
      }   
      applyRightHits(&gameState, &gameSettings,td, current_time_millis );    
      gameState.lastHit = td;
      Log.warningln("Right Trigger, hits=%d, sampletime = %l",td.hits , rightScanner.sampleTimeMillis );
      enable(&rightScanner);
  } 
}

void refreshDisplay(){
  nav.doInput();
  oled.setFont(u8g2_font_7x13_mf);
  oled.firstPage();
  do nav.doOutput(); while (oled.nextPage() );
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
        refreshDisplay();
        setAllMeterColors(CRGB::Black, CRGB::Black);
        setAllMetersToValue(0);
      }
  }
  else if ( programMode == PROGRAM_MODE_MENU){
    //TODO: package into the driver itself
      menuDriver.update();
      if ( menuDriver.dirty){
          refreshDisplay();
      }
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