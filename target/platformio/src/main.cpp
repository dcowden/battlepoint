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
#include <constants.h>
#include <math.h>
#include <EEPROM.h>
#include <ArduinoLog.h>

//Menu Includes
#include <menu.h>
#include <menuIO/keyIn.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIO.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>


#define VERTICAL_LED_SIZE 16
#define HORIONTAL_LED_SIZE 10
#define WINNER_SPLASH_MS 5000
#define GAME_UPDATE_INTERVAL_MS 50
#define EEPROM_SIZE 320


RealClock gameClock = RealClock();

//the different types of games we can play
GameState gameState;
GameSettings gameSettings;


CRGB leftLeds[VERTICAL_LED_SIZE];
CRGB centerLeds[VERTICAL_LED_SIZE];
CRGB rightLeds[VERTICAL_LED_SIZE];
CRGB topLeds[2* HORIONTAL_LED_SIZE];
CRGB bottomLeds[2* HORIONTAL_LED_SIZE];


//prototypes

void updateDisplay();
void menuIdleEvent();
void updateGame();
void startGame();
Menu::result doStartGame();

Ticker updateDisplayTimer(updateDisplay,DISPLAY_UPDATE_INTERVAL_MS);
Ticker gameUpdateTimer(updateGame, GAME_UPDATE_INTERVAL_MS );
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0,  I2C_SDA,I2C_SCL);


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

int getSlotAddress(int slot_num){
  return slot_num*sizeof(GameSettings);
}

void saveSettingSlot (int slot_num){
  int addr = getSlotAddress(slot_num);
  Log.noticeln("Saving Slot %d, addr %d, version= %d, winby=%d",slot_num,addr, gameSettings.BP_VERSION,gameSettings.hits.victory_margin);
  EEPROM.put(addr, gameSettings);
  EEPROM.commit();
}

void loadSettingSlot(int slot_num){

  GameSettings s;
  int addr = getSlotAddress(slot_num);
  Log.noticeln("Loading Slot %d, addr %d",slot_num,addr);

  EEPROM.get(addr,s);
  Log.noticeln("Loaded WinByHits= %d",s.hits.victory_margin);
  if ( s.BP_VERSION == BP_CURRENT_SETTINGS_VERSION){
    Log.noticeln("Found Valid Settings Version %d in slot %d. Returning Default",s.BP_VERSION, slot_num);
    gameSettings = s;
  }
  else{
    Log.noticeln("Found Invalid Settings Version %d in slot %d. Returning Data",s.BP_VERSION, slot_num);
    gameSettings = DEFAULT_GAMESETTINGS();
  }
}

void setupTargets(){
  pinMode(Pins::TARGET_LEFT,INPUT);
  pinMode(Pins::TARGET_RIGHT,INPUT);
}


Menu::result loadMostHitsSettings(){
  loadSettingSlot(GameSettingSlot::SLOT_1);
  return Menu::proceed;
}

Menu::result loadFirstToHitsSettings(){
  loadSettingSlot(GameSettingSlot::SLOT_2);
  return Menu::proceed;
}
Menu::result loadOwnZoneSettings(){
  loadSettingSlot(GameSettingSlot::SLOT_3);
  return Menu::proceed;
}
Menu::result loadCPSettings(){
  loadSettingSlot(GameSettingSlot::SLOT_4);
  return Menu::proceed;
}

Menu::result startLoadMostHitsGame(){
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME;
  saveSettingSlot(GameSettingSlot::SLOT_1);
  startGame();
  return Menu::quit;
}

Menu::result startFirstToHitsGame(){
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_HITS;
  saveSettingSlot(GameSettingSlot::SLOT_2);
  startGame();
  return Menu::quit;
}

Menu::result startOwnZoneGame(){
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME;
  saveSettingSlot(GameSettingSlot::SLOT_3);
  startGame();
  return Menu::quit;
}

Menu::result startCPGame(){
  gameSettings.gameType = GameType::GAME_TYPE_ATTACK_DEFEND;
  saveSettingSlot(GameSettingSlot::SLOT_4);
  startGame();
  return Menu::quit;
}

MENU(mostHitsSubMenu, "MostHits", loadMostHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",0,10,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startLoadMostHitsGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(firstToHitsSubMenu, "FirstToHits", loadFirstToHitsSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.to_win,"Hits","",0,100,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.hits.victory_margin,"Win By Hits","",0,10,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startFirstToHitsGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(ozSubMenu, "OwnZone", loadOwnZoneSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.capture.capture_cooldown_seconds,"Capture Cooldown","s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,FIELD(gameSettings.capture.capture_decay_rate_secs_per_hit,"Capture Decay","/s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.capture.hits_to_capture,"HitsToCapture","",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.capture.capture_offense_to_defense_ratio,"DefenseOffenseRatio","",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.ownership_time_seconds,"OwnTimeToWin","s",1,1,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle) 
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startOwnZoneGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(adSubMenu, "Capture", loadCPSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(gameSettings.hits.to_win,"Hits","",0,100,1,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(gameSettings.timed.max_duration_seconds,"Time Limit","s",10,1000,10,1,Menu::doNothing,Menu::noEvent,Menu::noStyle)    
    ,OP("Start",startCPGame, Menu::enterEvent)
    ,EXIT("<Back")
);

MENU(mainMenu, "BP Target v0.4", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,SUBMENU(mostHitsSubMenu)
  ,SUBMENU(ozSubMenu)
  ,SUBMENU(firstToHitsSubMenu)
  ,SUBMENU(adSubMenu)
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

void startGame(){  
  Log.noticeln("Starting Game. Type= %d", gameSettings.gameType);
  gameState = startGame(gameSettings, &gameClock);
  oled.clear();
  nav.idleOn();
}


void stopGameAndReturnToMenu(){

  oled.setFont(u8g2_font_7x13_mf); 
  oled.clear();
  nav.idleOff();  
  nav.idleOff();   
  nav.refresh();
}

Menu::result menuIdleEvent(menuOut &o, idleEvent e) {
  switch (e) {
    case idleStart:{
      Log.notice("Suspending Menu");
      oled.clear(); 
      updateDisplay();
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

  MeterSettings ms = gameState.meters;
  updateController(leftLeds, ms.left, gameClock.milliseconds());
  updateController(centerLeds, ms.center, gameClock.milliseconds());
  updateController(rightLeds, ms.right, gameClock.milliseconds());
  updateController(topLeds, ms.leftTop, gameClock.milliseconds());
  updateController(topLeds, ms.rightTop, gameClock.milliseconds());
  updateController(bottomLeds, ms.leftBottom, gameClock.milliseconds());
  updateController(bottomLeds, ms.rightBottom , gameClock.milliseconds());
  FastLED.show();

}

void displayWelcomeBanner( ){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.setCursor(0,40);
  oled.print("Target v0.2");   
  oled.sendBuffer();
  delay(SPLASH_WAIT_MS);    
}

void initDisplay(){
  oled.begin();
  displayWelcomeBanner();
  oled.clear();
  oled.setFont(u8g2_font_6x13_tf);
}

void setup() {

  Serial.begin(115200);
  Serial.setTimeout(500);
  EEPROM.begin(EEPROM_SIZE);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
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
void gameOverDisplay(){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.setCursor(0,40);
  oled.print("WIN: "); 
  oled.setCursor(50,40);  
  oled.print(teamTextChar(gameState.result.winner));
  oled.sendBuffer();
  delay(WINNER_SPLASH_MS);
  oled.setFont(u8g2_font_7x13_mf);   
}
void updateGame(){
  SensorState sensorState;
  sensorState.rightScan = check_target(readRightTarget,gameSettings.target,(Clock*)(&gameClock));
  sensorState.leftScan = check_target(readLeftTarget,gameSettings.target,(Clock*)(&gameClock));  
  updateGame(&gameState, sensorState, gameSettings, (Clock*)(&gameClock));

  if ( gameState.status == GameStatus::GAME_STATUS_ENDED ){
    Log.warning("Game Over!");
    Log.warning("Winner=");
    gameOverDisplay();
    stopGameAndReturnToMenu();
  }
  updateLEDs();
}

void updateDisplay(){
  oled.clearBuffer();
  oled.setCursor(5,15);
  oled.print("HT: R="); oled.print(gameState.hits.red_hits); oled.print( "  B="); oled.print(gameState.hits.blu_hits);

  long elapsed_millis = millis() - gameState.time.start_time_millis;
  int elapsed_sec = elapsed_millis/1000;
  oled.setCursor(5,27);
  oled.print("T: "); oled.print(elapsed_sec); oled.print("/"); oled.print(gameSettings.timed.max_duration_seconds); 
  oled.print(" ["); oled.print(getCharForStatus(gameState.status)); oled.print("]");
  oled.setCursor(5,39);
  oled.print("CAP: "); oled.print(gameState.ownership.capture_hits); oled.print("/"); oled.print(gameSettings.capture.hits_to_capture);
  oled.setCursor(5,52);
  oled.print("OWN: B=");oled.print(gameState.ownership.blu_millis/1000);oled.print("  R="); oled.print(gameState.ownership.red_millis/1000);
  oled.sendBuffer();
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