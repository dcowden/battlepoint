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

RealClock gameClock = RealClock();

//the different types of games we can play
GameType currentGameType = GameType::GAME_TYPE_KOTH_FIRST_TO_HITS;

FirstToOwnTimeGame ozGame;
AttackDefendGame adGame;
MostOwnInTimeGame mostTimeGame;
MostHitsInTimeGame mostHitsGame;
FirstToHitsGame firstHitsGame;

CRGB leftLeds[VERTICAL_LED_SIZE];
CRGB centerLeds[VERTICAL_LED_SIZE];
CRGB rightLeds[VERTICAL_LED_SIZE];
CRGB topLeds[2* HORIONTAL_LED_SIZE];
CRGB bottomLeds[2* HORIONTAL_LED_SIZE];

//prototypes

void updateDisplay();
void menuIdleEvent();

Ticker updateDisplayTimer(updateDisplay,DISPLAY_UPDATE_INTERVAL_MS);
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
  if ( currentGameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){

  }  
}

void stopGame(){
  appMode = APP_MENU_MODE;
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

MENU(mainMenu, "BP Target v0.2", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
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
  { -Pins::MOUSE_UP, Menu::options->getCmdChar(upCmd )} ,
  { -Pins::MOUSE_DOWN, Menu::options->getCmdChar(downCmd )} ,
  //{ -PIN_LEFT, Menu::options->getCmdChar(escCmd )}  ,
  //{ -PIN_RIGHT, Menu::options->getCmdChar(enterCmd )}  ,
  { -Pins::MOUSE_CENTER, Menu::options->getCmdChar(enterCmd )}  ,
};

Menu::keyIn<5> thingy_buttons(btn_map);

MENU_OUTPUTS(out,MENU_MAX_DEPTH
  ,U8G2_OUT(oled,colors,fontW,fontH,OFFSET_X,OFFSET_Y,{0,0,charWidth,lineHeight})
  ,SERIAL_OUT(Serial)
);

NAVROOT(nav,mainMenu,MENU_MAX_DEPTH,thingy_buttons,out);

Menu::result menuIdleEvent(menuOut &o, idleEvent e) {
  switch (e) {
    case idleStart:{
      Serial.println("suspending menu!"); 
      appMode=APP_GAME_RUNNING; 
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
       appMode=APP_MENU_MODE;
       oled.clear();        
       nav.reset();
       nav.refresh();
       break;
    }
  }
  return proceed;
}
void updateLEDs(){

  FastLED.show();
}

void displayWelcomeBanner( ){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.setCursor(0,40);
  oled.print("BP Target v0.2");   
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

  thingy_buttons.begin();
  // hardwareOutputTimer.start();
  updateDisplayTimer.start();
  nav.idleTask = menuIdleEvent;

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

void updateGame(){
  if ( currentGameType == GameType::GAME_TYPE_KOTH_FIRST_TO_HITS){
    TargetSettings ts = firstHitsGame.settings.targetSettings;
    TargetHitScanResult right_result = check_target(readRightTarget,ts,(Clock*)(&gameClock));
    TargetHitScanResult left_result = check_target(readLeftTarget,ts,(Clock*)(&gameClock));  
    update(&firstHitsGame, left_result, right_result, (Clock*)(&gameClock));

  }
}

void loop() {
  //updateTargets();
  updateGame();
  updateLEDs();  
  nav.doInput();
  
  //user is in a menu
  if ( appMode == APP_MENU_MODE ){
    if ( nav.changed(0) ){
      oled.firstPage();
      do nav.doOutput(); while (oled.nextPage() );
    }
  }

  //menu is suspended, app is running
  else if ( appMode == APP_GAME_RUNNING){

    updateDisplayTimer.update();
  }
  else{
    Serial.println("Unknown Mode");
  }
}