#include <Arduino.h>
#include "Ticker.h"
#include <U8g2lib.h>
#include <FastLED.h>
#include <LedMeter.h>
#include <game.h>
//Menu Includes
#include <menuIO/keyIn.h>
#include <menu.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>


//pins
#define I2C_ADDRESS 0x3C
#define I2C_SDA 21
#define I2C_SCL 22
#define PIN_UP 36
#define PIN_DOWN 34
#define PIN_LEFT 33
#define PIN_RIGHT 32
#define PIN_CENTER 39


#define HIT_PIN 25
#define NUM_LEDS 60
#define LED_PIN 12

//app constants
#define SPLASH_WAIT_MS 1000
#define MENU_MAX_DEPTH 2
#define OFFSET_X 0
#define OFFSET_Y 0
#define U8_WIDTH 128
#define U8_HEIGHT 64
#define DISPLAY_UPDATE_INTERVAL_MS 500
#define HITS_TO_WIN 14

CRGB leds[NUM_LEDS];
LedRange targetMeterRange [1] = {  { 1, 12 } } ; //
LedMeter targetMeter = LedMeter(leds,targetMeterRange,1,CRGB::Blue, CRGB::Black);
short fontW = 6;
short fontH = 13;

//prototypes
//void updateDryer();
void updateDisplay();
void menuIdleEvent();

Ticker updateDisplayTimer(updateDisplay,DISPLAY_UPDATE_INTERVAL_MS);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0,  I2C_SDA,I2C_SCL);

enum AppModeValues{
  APP_MENU_MODE,
  APP_GAME_RUNNING
};

int appMode = APP_MENU_MODE;
int num_hits = 0;


void displayWelcomeBanner(){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  fontW = 10;
  fontH = 16;
  oled.setCursor(0,40);
  oled.print("BP Target v0.1");   
  oled.sendBuffer();
  delay(SPLASH_WAIT_MS);
}

void setupLEDs(){
  pinMode(LED_PIN,OUTPUT);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
}

void setupOLED(){
  oled.begin();
  displayWelcomeBanner();
  oled.clear();
  oled.setFont(u8g2_font_6x13_tf);
  fontW = 6;
  fontH = 13;
}

void startGame() {
    //targetMeter.setMaxValue(HITS_TO_WIN);
    //targetMeter.setToMin();
    //targetMeter.setColors(CRGB::Red,CRGB::Black);
    oled.clear();    
    appMode = APP_GAME_RUNNING;
}

void stopGame(){
  oled.clear();    
  appMode = APP_MENU_MODE;
}

char charForBoolean(boolean v){
  if (v){
    return 'Y';
  }
  else{
    return 'N';
  }
}

int getLineLocation(int linenum){
  return (int)(linenum * fontH) + 1;
}

void updateDisplay(){  
  oled.clearBuffer();
  oled.setCursor(0,getLineLocation(1));
  oled.print("Hits: ");
  oled.print(num_hits);
    oled.setCursor(0,getLineLocation(2));
  oled.print("Rqd: ");
  oled.print(HITS_TO_WIN);
  oled.sendBuffer();
}

Menu::result doStartGame() {
  oled.clear();
  startGame();
  appMode = APP_GAME_RUNNING;
  return Menu::quit;
}

Menu::result doStop() {
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

MENU(mainMenu, "BP Target v0.1", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
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
  { -PIN_UP, Menu::options->getCmdChar(upCmd )} ,
  { -PIN_DOWN, Menu::options->getCmdChar(downCmd )} ,
  { -PIN_LEFT, Menu::options->getCmdChar(escCmd )}  ,
  { -PIN_RIGHT, Menu::options->getCmdChar(enterCmd )}  ,
  { -PIN_CENTER, Menu::options->getCmdChar(enterCmd )}  ,
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

void handle_hit() {
  num_hits++;
}

void setup() {

  Serial.begin(115200);
  Serial.setTimeout(500);
  Serial.println("Starting...");
  setupOLED();
  Serial.println("OLED [OK]");
  setupLEDs();    
  Serial.println("LEDS [OK]");  
  pinMode(HIT_PIN,INPUT);
  attachInterrupt(digitalPinToInterrupt(HIT_PIN),handle_hit,RISING);
  thingy_buttons.begin();
  // hardwareOutputTimer.start();
  updateDisplayTimer.start();
  nav.idleTask = menuIdleEvent;

}

void updateLEDs(){
  targetMeter.setValue ( num_hits );
  FastLED.show();
}


void stopTimers(){
  updateDisplayTimer.stop();
}

void loop() {
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