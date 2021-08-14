#include <Arduino.h>
#include "Ticker.h"
#include <U8g2lib.h>
#include <FastLED.h>
#include <target.h>
#include <Clock.h>
#include <LedMeter.h>
#include <game.h>
#include <display.h>
#include <util.h>
#include <pins.h>
#include <math.h>
#include <ArduinoLog.h>
#include <settings.h>

//Menu Includes
#include <menu.h>
#include <menuIO/keyIn.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIO.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>

#define MENU_MAX_DEPTH 4
#define OFFSET_X 0
#define OFFSET_Y 0
#define DISPLAY_UPDATE_INTERVAL_MS 500
#define VERTICAL_LED_SIZE 16
#define HORIONTAL_LED_SIZE 10
#define GAME_UPDATE_INTERVAL_MS 50

RealClock gameClock = RealClock();

//the different types of games we can play
GameState gameState;
GameSettings gameSettings;

//the currently selected game
GameType selectedGameType = GameType::GAME_TYPE_UNSELECTED;

CRGB leftLeds[VERTICAL_LED_SIZE];
CRGB centerLeds[VERTICAL_LED_SIZE];
CRGB rightLeds[VERTICAL_LED_SIZE];
CRGB topLeds[2* HORIONTAL_LED_SIZE];
CRGB bottomLeds[2* HORIONTAL_LED_SIZE];

//prototypes
//void updateDisplay();
void menuIdleEvent();
void updateGame();
void startSelectedGame();


void updateDisplayLocal(){
  updateDisplay(gameState,gameSettings);
}

Ticker updateDisplayTimer(updateDisplayLocal,DISPLAY_UPDATE_INTERVAL_MS);
Ticker gameUpdateTimer(updateGame, GAME_UPDATE_INTERVAL_MS );

void setupLEDs(){
  pinMode(Pins::LED_TOP,OUTPUT);
  pinMode(Pins::LED_BOTTOM,OUTPUT);
  pinMode(Pins::LED_LEFT_EDGE,OUTPUT);
  pinMode(Pins::LED_CENTER_VERTICAL,OUTPUT);
  pinMode(Pins::LED_RIGHT_EDGE,OUTPUT);

  FastLED.addLeds<NEOPIXEL, Pins::LED_TOP>(topLeds, 2* HORIONTAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_BOTTOM>(bottomLeds, 2* HORIONTAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_LEFT_EDGE>(leftLeds, VERTICAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_CENTER_VERTICAL>(centerLeds, VERTICAL_LED_SIZE);
  FastLED.addLeds<NEOPIXEL, Pins::LED_RIGHT_EDGE>(rightLeds, VERTICAL_LED_SIZE);
}

void loadSettingsForSelectedGameType(){
    loadSettingSlot(&gameSettings, getSlotForGameType(selectedGameType));
}

void saveSettingsForSelectedGameType(){
   saveSettingSlot(&gameSettings, getSlotForGameType(selectedGameType));
}

void setupTargets(){
  pinMode(Pins::TARGET_LEFT,INPUT);
  pinMode(Pins::TARGET_RIGHT,INPUT);
}

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
  gameSettings.hits.to_win = 10;
  return Menu::proceed;
}

MENU(mostHitsSubMenu, "MostHits", loadMostHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",0,10,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(firstToHitsSubMenu, "FirstToHits", loadFirstToHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.to_win,"Hits","",0,100,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",0,10,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle)         
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(ozSubMenu, "OwnZone", loadOwnZoneSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.capture.capture_cooldown_seconds,"Capture Cooldown","s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.capture.capture_decay_rate_secs_per_hit,"Capture Decay","/s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.capture.hits_to_capture,"HitsToCapture","",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.ownership_time_seconds,"OwnTimeToWin","s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle)     
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(adSubMenu, "Capture", loadCPSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.to_win,"Hits","",0,100,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(testTargetMenu, "TargetTest",  loadTargetTestSettings, Menu::enterEvent, Menu::wrapStyle    
    ,FIELD(gameSettings.target.hit_energy_threshold,"Hit Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.target.trigger_threshold,"Trig Thresh","",100,5000,1000,100,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,OP("Start",startSelectedGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(mainMenu, "BP Target v0.4", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,SUBMENU(mostHitsSubMenu)
  ,SUBMENU(ozSubMenu)
  ,SUBMENU(firstToHitsSubMenu)
  ,SUBMENU(adSubMenu)
  ,SUBMENU(testTargetMenu)
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

Menu::keyMap btn_map[] = {
  { -39, Menu::options->getCmdChar(enterCmd) }, 
  { -34, Menu::options->getCmdChar(downCmd) } ,
  { -36, Menu::options->getCmdChar(upCmd) }
};

Menu::keyIn<4> thingy_buttons(btn_map);

MENU_INPUTS(in,&thingy_buttons);
MENU_OUTPUTS(out,MENU_MAX_DEPTH
  ,U8G2_OUT(oled,colors,fontW,fontH,OFFSET_X,OFFSET_Y,{0,0,charWidth,lineHeight})
  ,SERIAL_OUT(Serial)
);

NAVROOT(nav,mainMenu,MENU_MAX_DEPTH,thingy_buttons,out);


MeterSettings get_base_meters(){
    MeterSettings s;        
    initMeter(&s.leftTop.meter,"leftTop",topLeds,0,9);
    initMeter(&s.leftBottom.meter,"leftBottom",bottomLeds,0,9);
    initMeter(&s.rightTop.meter,"rightTop",topLeds,10,19);
    initMeter(&s.rightBottom.meter,"rightBottom",bottomLeds,10,19);
    initMeter(&s.center.meter,"center",centerLeds,0,15);
    initMeter(&s.left.meter,"left",leftLeds,0,15);
    initMeter(&s.right.meter,"right",rightLeds,0,15);
    return s;
}

void startSelectedGame(){  
  Log.noticeln("Starting Game. Type= %d", gameSettings.gameType);
  saveSettingsForSelectedGameType();
  gameState = startGame(gameSettings, &gameClock,get_base_meters());
  oled.clear();
  nav.idleOn();
}

void stopGameAndReturnToMenu(){
  oled.setFont(u8g2_font_7x13_mf); 
  oled.clear();
  nav.idleOff();  
  nav.refresh();
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
       stopGameAndReturnToMenu();       
       break;
    }
  }
  return proceed;
}

void updateLEDs(){
  updateLeds(&gameState, gameClock.milliseconds());
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(500);
  initSettings();
  Log.begin(LOG_LEVEL_WARNING, &Serial, true);
  Log.warning("Starting...");
  initDisplay();
  Log.notice("OLED [OK]");
  setupLEDs();    
  Log.notice("LEDS [OK]");  
  setupTargets();
  Log.notice("TARGETS [OK]");
  thingy_buttons.begin();
  // hardwareOutputTimer.start();
  updateDisplayTimer.start();
  gameUpdateTimer.start();
  //nav.idleTask = menuIdleEvent;
  options->invertFieldKeys = true;
  Log.warningln("Complete.");
  oled.setFont(u8g2_font_7x13_mf);

  gameSettings = DEFAULT_GAMESETTINGS();
}

void stopTimers(){
  updateDisplayTimer.stop();
}

int readLeftTarget(){
   return analogRead(Pins::TARGET_LEFT);
}

int readRightTarget(){
  return analogRead(Pins::TARGET_RIGHT);
}

SensorState readTargets(TargetSettings targetSettings){
  SensorState sensorState;
  sensorState.rightScan = check_target(readRightTarget,targetSettings,(Clock*)(&gameClock));
  sensorState.leftScan = check_target(readLeftTarget,targetSettings,(Clock*)(&gameClock));  
  return sensorState;
}

void updateGame(){
  SensorState s = readTargets(gameSettings.target);
  updateGame(&gameState, s, gameSettings, (Clock*)(&gameClock));
  if ( gameState.status == GameStatus::GAME_STATUS_ENDED ){
    Log.warning("Game Over!");
    Log.warning("Winner=");
    gameOverDisplay(gameState);
    stopGameAndReturnToMenu();
  }
  updateLEDs();
}

void loop() {  
  nav.doInput();
  if ( nav.sleepTask){
    gameUpdateTimer.update();
    updateDisplayTimer.update();
  }
  else{
    if ( nav.changed(0) ){
      oled.setFont(u8g2_font_7x13_mf);
      oled.firstPage();
      do nav.doOutput(); while (oled.nextPage() );
    }
  }
}