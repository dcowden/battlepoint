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

//Menu Includes
#include <menuIO/keyIn.h>
#include <menu.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>

#define BP_DEBUG 1
#define VERTICAL_LED_SIZE 16
#define HORIONTAL_LED_SIZE 10
#define WINNER_SPLASH_MS 2000
#define GAME_UPDATE_INTERVAL_MS 50
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

Ticker updateDisplayTimer(updateDisplay,DISPLAY_UPDATE_INTERVAL_MS);
Ticker gameUpdateTimer(updateGame, GAME_UPDATE_INTERVAL_MS );
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0,  I2C_SDA,I2C_SCL);

enum AppModeValues{
  APP_MENU_MODE,
  APP_GAME_RUNNING
};

int appMode = APP_MENU_MODE;

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

void setupTargets(){
  pinMode(Pins::TARGET_LEFT,INPUT);
  pinMode(Pins::TARGET_RIGHT,INPUT);
}

void startGame() {
  gameSettings.hits.to_win = 10;
  gameSettings.hits.to_capture = 10;
  gameSettings.hits.victory_margin = 2;
  gameSettings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_HITS;
  gameState = startGame(gameSettings, (Clock*)(&gameClock) );
  Serial.println("Starting Game.");
  Serial.print("M=");
  Serial.println(gameState.meters.center.max_val);
}



Menu::result doStartGame() {
  startGame();
  appMode = APP_GAME_RUNNING;
  return Menu::quit;
}

Menu::result doStop() {
  appMode = APP_MENU_MODE;
  return Menu::quit;
}

/**
Menu::result saveSettings(){
  return quit;
}


 * TOGGLE(dryerOptions.enableFans,setFansToggle,"Fans: ", Menu::doNothing, Menu::noEvent, Menu::noStyle
  ,VALUE("ON",1,Menu::doNothing,Menu::noEvent)
  ,VALUE("OFF",0,Menu::doNothing,Menu::noEvent)
);  

TOGGLE(dryerOptions.enableHeater,setHeatersToggle,"Heaters: ", Menu::doNothing, Menu::noEvent, Menu::noStyle
  ,VALUE("ON",1,Menu::doNothing,Menu::noEvent)
  ,VALUE("OFF",0,Menu::doNothing,Menu::noEvent)
);


MENU(settingsSubMenu, "Settings", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
    ,SUBMENU(setFansToggle)
    ,SUBMENU(setHeatersToggle)
    ,OP("Save",saveSettings, Menu::enterEvent)
);

CHOOSE(profileSelectionIndex,presetMenu,"Filament:",Menu::doNothing,noEvent,noStyle
  ,VALUE("PLA",0,Menu::doNothing,noEvent)
  ,VALUE("ABS",1,Menu::doNothing,noEvent)
  ,VALUE("PETG",2,Menu::doNothing,noEvent)
  ,VALUE("NYLON",3,Menu::doNothing,noEvent)
  ,VALUE("PVA",4,Menu::doNothing,noEvent)
  ,VALUE("TPU/TPE",5,Menu::doNothing,noEvent)
  ,VALUE("ASA",6,Menu::doNothing,noEvent)
  ,VALUE("PP",7,Menu::doNothing,noEvent)
  ,VALUE("Test",8,Menu::doNothing,noEvent)
);
**/

MENU(mainMenu, "BP Target v0.3", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  //,SUBMENU(presetMenu)
  //,SUBMENU(settingsSubMenu)
  ,OP("START",doStartGame,Menu::enterEvent)
  ,OP("STOP",doStop,Menu::enterEvent)
);

const colorDef<uint8_t> colors[6] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,1}},//cursorColor
  {{1,1},{1,0,0}},//titleColor
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
  { -34, Menu::options->getCmdChar(upCmd) } ,
  { -36, Menu::options->getCmdChar(downCmd) } ,
};

Menu::keyIn<3> thingy_buttons(btn_map);

MENU_INPUTS(in,&thingy_buttons);
MENU_OUTPUTS(out,MENU_MAX_DEPTH
  ,U8G2_OUT(oled,colors,fontW,fontH,OFFSET_X,OFFSET_Y,{0,0,charWidth,lineHeight})
  ,SERIAL_OUT(Serial)
);

NAVROOT(nav,mainMenu,MENU_MAX_DEPTH,thingy_buttons,out);

void stopGame(){
  appMode = APP_MENU_MODE;
  oled.setFont(u8g2_font_7x13_mf); 
  nav.idleOff();   
  nav.refresh();
}

Menu::result menuIdleEvent(menuOut &o, idleEvent e) {
  switch (e) {
    case idleStart:{
      Serial.println("suspending menu!"); 
      //appMode=APP_GAME_RUNNING; 
      oled.clear(); 
      updateDisplay();
      break;
    } 
    case idling:{
      Serial.println("suspended..."); 
      break;
    } 
    case idleEnd:{
       Serial.println("resuming menu."); 
       stopGame();
       oled.clear();        
       nav.reset();
       nav.refresh();
       break;
    }
  }
  return proceed;
}
void updateLEDs(){

  MeterSettings ms = gameState.meters;
  updateLedMeter(leftLeds, ms.left);
  updateLedMeter(centerLeds, ms.center);
  updateLedMeter(rightLeds, ms.right);
  updateLedMeter(topLeds, ms.leftTop);
  updateLedMeter(topLeds, ms.rightTop);
  updateLedMeter(bottomLeds, ms.leftBottom);
  updateLedMeter(bottomLeds, ms.rightBottom );
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
  Serial.println("Starting...");
  initDisplay();
  Serial.println("OLED [OK]");
  setupLEDs();    
  Serial.println("LEDS [OK]");  
  setupTargets();
  Serial.println("TARGETS [OK]");
  thingy_buttons.begin();
  // hardwareOutputTimer.start();
  updateDisplayTimer.start();
  gameUpdateTimer.start();
  //nav.idleTask = menuIdleEvent;
  Serial.println("Complete.");
  oled.setFont(u8g2_font_7x13_mf);
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
    Serial.println("Game Over!");
    Serial.print("Winner=");
    gameOverDisplay();
    stopGame();
  }
  updateLEDs();
}

void updateDisplay(){
  //super simple for now
  //TODO: needs to be moved to a game method
  oled.clearBuffer();
  /**
  oled.print(getCharForGameType(gameSettings.gameType));
  oled.print(":");
  switch ( gameState.status ){
    case GameStatus::GAME_STATUS_RUNNING:
      oled.println("RUNNING");  
      break;
    case GameStatus::GAME_STATUS_OVERTIME:
      oled.println("OVERTIME");  
      break;
    case GameStatus::GAME_STATUS_ENDED:
      oled.println("ENDED");  
      break;
  }**/
  oled.setCursor(5,15);
  oled.print("HT: R="); oled.print(gameState.hits.red_hits,2); oled.print( "  B="); oled.print(gameState.hits.blu_hits);

  long elapsed_millis = millis() - gameState.time.start_time_millis;
  int elapsed_sec = elapsed_millis/1000;
  oled.setCursor(5,27);
  oled.print("T: "); oled.print(elapsed_sec); oled.print("/"); oled.print(gameSettings.timed.game_duration_seconds);
  oled.setCursor(5,39);
  oled.print("CAP: "); oled.print(gameState.ownership.capture_hits); oled.print("/"); oled.print(gameSettings.capture.hits_to_capture);
  oled.setCursor(5,52);
  oled.print("OWN: B=");oled.print(gameState.ownership.blu_millis/1000);oled.print("  R="); oled.print(gameState.ownership.red_millis/1000);
  oled.sendBuffer();
}

void loop() {  
  nav.doInput();
  //user is in a menu
  if ( appMode == APP_MENU_MODE ){
    if ( nav.changed(0) ){
      oled.setFont(u8g2_font_7x13_mf);
      oled.firstPage();
      do nav.doOutput(); while (oled.nextPage() );
    }
  }
  //menu is suspended, app is running
  else if ( appMode == APP_GAME_RUNNING){
    gameUpdateTimer.update();
    updateDisplayTimer.update();
  }
  else{
    Serial.println("Unknown Mode");
  }
}