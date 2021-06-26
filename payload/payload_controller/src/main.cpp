#include "Arduino.h"
#include <FastLED.h>
#include "LedMeter.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "MotorCommands.h"
#include "I2CAnything.h"
#include "Payload.h"
#include <EEPROM.h>
#include "OneButton.h"
#include <ESP32Encoder.h>
#include "EncoderMenuDriver.h"
#include "Ticker.h"
#include "pinmap.h"
#include "BLEScanner.h"

//Menu Includes
#include <menuIO/keyIn.h>
#include <menu.h>
#include <menuIO/SSD1306AsciiOut.h>
#include <menuIO/serialOut.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>

#define menuFont X11fixed7x14
#define fontW 7
#define fontH 14
#define MENU_MAX_DEPTH 2

//app constants
#define NUM_LEDS 60
#define I2C_ADDRESS 0x3C
#define GAME_OVER_DANCE_SECS 10
#define GAME_OVER_DANCE_DELAY_MS 100
#define DISPLAY_UPDATE_INTERVAL_MS 200
#define SERIAL_UPDATE_INTERVAL_MS 1000
#define MOTOR_UPDATE_INTERVAL_MS 50
#define DEBUG 1
#define EEPROMStartAddress 0 // Where to start reading and writing EEPROM

struct gameOptions tmpGameOptions; // Temorary struct for reading EEPROM into
struct gameOptions gameOptions = {
    14,   //firmward version   
    600,   //default game length
    1700,  //fwd speed x1
    3000,  //fwd speed x2
    4000,  //fwd speed x3
    -1500,  //bwd speed
    -55     //token rssi threshold
}; 

enum GameResults
{
  CANCELLED,
  ATTACK_WIN,
  DEFEND_WIN
};

//function prototypes
void readGameSettings();
void writeGameSettings();
void updateDisplay();
void updateLEDs();
void updateSerial();
void encButton_SingleClick();
void encButton_DoubleClick();


CRGB leds[NUM_LEDS];
SSD1306AsciiWire oled;
ESP32Encoder encoder;
LedRange payloadRange [2] = {  { 1, 29 } , { 58,30 }} ; //
LedMeter payloadMeter = LedMeter(leds,payloadRange,2,CRGB::Blue, CRGB::Black);
Payload payload;
Game game(&payload);

OneButton encButton(ROTARY_ENCODER_BUTTON_PIN, true);
BLEProximityScanner scanner;

TaskHandle_t BLETask;

void sendMotorCommand(){
    Wire.beginTransmission (MOTOR_CONTROLLER_ADDRESS );
    I2C_writeAnything (payload.lastCommand.leftVelocity);
    I2C_writeAnything (payload.lastCommand.rightVelocity);
    I2C_writeAnything (payload.lastCommand.enabled);
    Wire.endTransmission (); 
}

Ticker displayTimer(updateDisplay,DISPLAY_UPDATE_INTERVAL_MS,0,MILLIS);
Ticker serialTimer(updateSerial,SERIAL_UPDATE_INTERVAL_MS,0,MILLIS);
Ticker motorCommandTimer(sendMotorCommand,MOTOR_UPDATE_INTERVAL_MS,0,MILLIS);

enum AppModeValues
{
  APP_GAME_RUNNING,
  APP_MENU_MODE,
  APP_PROCESS_MENU_CMD,
  APP_MANUAL_DRIVE_MODE
};
int appMode = APP_MENU_MODE;


void setupLEDs(){
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
}

void setupOLED(){
  Wire.setClock(400000L);  
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(menuFont);
  oled.displayRemap(true);
  oled.clear();
  oled.println("BattlePoint v1.0"); 
  oled.println("Starting..."); 
}

void startGame() {
    Serial.print("Game Starting: ");
    Serial.print(gameOptions.gameTimeSeconds);
    Serial.println( " secs.");
    game.options = gameOptions;
    payload.options = gameOptions;    
    payloadMeter.setMaxValue(gameOptions.gameTimeSeconds);
    payloadMeter.setToMax();
    payloadMeter.setColors(CRGB::Blue,CRGB::Black);
    oled.clear();    
    payload.enable();
    scanner.rssiThreshold = gameOptions.rssiThreshold;
    game.start();
    appMode = APP_GAME_RUNNING;
}

void setupEncoder(){
  encButton.attachDoubleClick(encButton_DoubleClick);
  encButton.attachClick(encButton_SingleClick);
  encButton.setDebounceTicks(80);
  encoder.attachHalfQuad(ROTARY_ENCODER_A_PIN,ROTARY_ENCODER_B_PIN);
}
void setupButtons(){
  pinMode(BTN_FWD_1,INPUT_PULLUP);
  pinMode(BTN_FWD_2,INPUT_PULLUP);
  pinMode(BTN_FWD_3,INPUT_PULLUP);
  pinMode(BTN_BWD,INPUT_PULLUP);
}
void setupSensors(){
  pinMode(WIRE_SENSOR_RIGHT,INPUT_PULLUP);
  pinMode(WIRE_SENSOR_LEFT,INPUT_PULLUP);
}

void setupEEProm(){
  EEPROM.begin(4096);
  readGameSettings();
}

void bleScanLoop(void * pvParameters){
  Serial.println("BLE Scanner Running");
  scanner.init();
  while (true){
     scanner.update();
     if ( DEBUG ){
        if ( scanner.foundDevice() ){
          Serial.println("FOUND IT. I FOUND IT!");
        }
        else{
          Serial.println("DINT FIND ANYTHING in BLE");
        }
     }
  }
}
void setupBLETask(){
  xTaskCreatePinnedToCore(
              bleScanLoop,  /* Task function. */
              "BLESCAN",    /* name of task. */
              5000,      /* Stack size of task */
              NULL,       /* parameter of the task */
              1,          /* priority of the task */
              &BLETask,     /* Task handle to keep track of created task */
              1);    
}

void startTimers(){
  displayTimer.start();
  serialTimer.start();
  motorCommandTimer.start();
}

void stopTimers(){
  displayTimer.stop();
  serialTimer.stop();
  motorCommandTimer.stop();
}
void setup() {
  Serial.begin(57600);
  Wire.begin();
  Wire.setClock(100000);  
  setupLEDs();
  setupOLED();
  setupEncoder();
  setupButtons();
  setupEEProm();
  startTimers();
  payload.disable();
  setupBLETask();
}

void updateLEDs(){
  payloadMeter.setValue ( game.getSecondsRemaining() );
  FastLED.show();
}

void gameOverDisplay(int result){
  
  long end_time = millis() + (GAME_OVER_DANCE_SECS*1000);
  if ( result == ATTACK_WIN){
    payloadMeter.setColors(CRGB::Blue,CRGB::Black);
  }
  else if ( result == DEFEND_WIN){
    payloadMeter.setColors(CRGB::Red,CRGB::Black);
  }
  else {
    payloadMeter.setColors(CRGB::Green,CRGB::Black);
  }
  payloadMeter.update();
  while ( millis() < end_time ){
    payloadMeter.setToMax();
    FastLED.show();
    FastLED.delay(GAME_OVER_DANCE_DELAY_MS);    
    payloadMeter.setToMin();
    FastLED.show();
    FastLED.delay(GAME_OVER_DANCE_DELAY_MS);    
  }
  Serial.println("Game Over!!");
}

void updateDisplay(){  
  char SPACE = ' ';
  oled.setCursor(0,0);
  oled.print("Time Rem: "); 
  oled.print(game.getSecondsRemaining());  
  oled.println(" s"); 
  oled.clearToEOL();
  oled.print("BTNS:");  
  oled.print(SPACE); 
  oled.print(payload.fwd_btn_1);
  oled.print(SPACE); 
  oled.print(payload.fwd_btn_2); 
  oled.print(SPACE); 
  oled.print(payload.fwd_btn_3);
  oled.print(SPACE); 
  oled.println(payload.bwd_btn_1); 
  oled.clearToEOL();
  oled.print("SEN: ");
  oled.print(payload.left_sensor);
  oled.print(" ");   
  oled.println(payload.right_sensor);
  oled.clearToEOL();
  oled.print("MTR: ");  
  oled.print((int)payload.lastCommand.leftVelocity);
  oled.print(" ");
  oled.print((int)payload.lastCommand.rightVelocity);
  oled.clearToEOL();

}

void updateSerial(){
   char SPACE = ' ';
   Serial.print("B:");
   Serial.print(SPACE);
   Serial.print(payload.fwd_btn_1);
   Serial.print(SPACE);
   Serial.print(payload.fwd_btn_2);
   Serial.print(SPACE);
   Serial.print(payload.fwd_btn_3);
   Serial.print(SPACE);
   Serial.print(payload.bwd_btn_1);
   Serial.print(SPACE);
   Serial.print("L:");
   Serial.print(payload.left_sensor);
   Serial.print(SPACE);
   Serial.print("R:");   
   Serial.print(payload.right_sensor);
   Serial.print(SPACE);     
   Serial.print("LM:");
   Serial.print(payload.lastCommand.leftVelocity);
   Serial.print(SPACE);   
   Serial.print("RM:");
   Serial.print(payload.lastCommand.rightVelocity);
   Serial.print(" EN:");
   Serial.print(payload.lastCommand.enabled);
   Serial.print(" Mode:");
   Serial.print(payload.manualDrive);
   Serial.print(" enabled:");
   Serial.print(payload.enabled);
   Serial.print(" BLE:");
   Serial.println(scanner.foundDevice());
   
}

Menu::result doStartGame() {
  Serial.println("Starting Game...");
  startGame();
  return proceed;
}

Menu::result menuUpdateEEPROM() {
  writeGameSettings();
  return quit;
}

Menu::result doStartManualControl(){
  Serial.println("Manual Mode Enabled");
  payload.manualDrive = true;  
  payload.enable();
  return proceed;
}

Menu::result doEndManualControl(){
  Serial.println("Auto Mode");
  payload.disable();
  payload.manualDrive = false;
  return proceed;
}

MENU(settingsSubMenu, "Settings", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
    ,FIELD(gameOptions.gameTimeSeconds,"Game Time","",0,1000,30,5, Menu::doNothing, Menu::noEvent, Menu::wrapStyle)
    ,FIELD(gameOptions.bwdSpeed_1,"BwdSpeed","",-4000,0,500,100, Menu::doNothing, Menu::noEvent, Menu::wrapStyle)
    ,FIELD(gameOptions.fwdSpeed_1,"FwdSpeed1X","",1000,6000,500,100, Menu::doNothing, Menu::noEvent, Menu::wrapStyle)
    ,FIELD(gameOptions.fwdSpeed_2,"FwdSpeed2X","",1000,6000,500,100, Menu::doNothing, Menu::noEvent, Menu::wrapStyle)
    ,FIELD(gameOptions.fwdSpeed_3,"FwdSpeed3X","",1000,6000,500,100, Menu::doNothing, Menu::noEvent, Menu::wrapStyle)
    ,FIELD(gameOptions.rssiThreshold,"RSSI","",-100,0,5,1, Menu::doNothing, Menu::noEvent, Menu::wrapStyle)
    ,OP("Save Settings",menuUpdateEEPROM, Menu::enterEvent)
    , EXIT("<Back")
);

int driveMode=1;
TOGGLE(driveMode,driveModeMenu,"Motors: ",doNothing,noEvent,wrapStyle
  ,VALUE("MAN",0,doStartManualControl,noEvent)
  ,VALUE("AUTO",1,doEndManualControl,noEvent)
);

MENU(mainMenu, "BP Payload v0.1", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,OP("Start Game!",doStartGame,Menu::enterEvent)
  ,SUBMENU(driveModeMenu)
  ,SUBMENU(settingsSubMenu)
);

Menu::serialIn serial(Serial);

//define output device
Menu::idx_t serialTops[MENU_MAX_DEPTH] = {0};
serialOut outSerial(Serial, serialTops);

//describing a menu output device without macros
//define at least one panel for menu output
constMEM Menu::panel panels[] MEMMODE = {{0, 0, 128 / fontW, 64 / fontH}};
Menu::navNode* nodes[sizeof(panels) / sizeof(Menu::panel)]; //navNodes to store navigation status
Menu::panelsList pList(panels, nodes, 1); //a list of panels and nodes
Menu::idx_t tops[MENU_MAX_DEPTH] = {0,0}; //store cursor positions for each level
SSD1306AsciiOut outOLED(&oled, tops, pList, fontW, 1+((fontH-1)>>3) ); //oled output device menu driver
menuOut* constMEM outputs[] MEMMODE = {&outOLED, &outSerial}; //list of output devices
outputsList out(outputs, sizeof(outputs) / sizeof(menuOut*)); //outputs list


//NAVROOT(nav,mainMenu,MENU_MAX_DEPTH,in,out);
NAVROOT(nav, mainMenu, MENU_MAX_DEPTH, serial, out);

EncoderMenuDriver encoderDriver ( &nav, &encoder, &encButton);
void encButton_SingleClick(){
  encoderDriver.button_clicked();
}
void encButton_DoubleClick(){
  encoderDriver.button_dbl_clicked();
} 
void loop() {
  
  payload.update();
  motorCommandTimer.update();  
  serialTimer.update();
  updateLEDs();

  if ( appMode == APP_GAME_RUNNING){
    //check button, so we can cancel
    encButton.tick();    
    displayTimer.update();
    if ( encButton.isLongPressed() || game.isOver() || scanner.foundDevice() ){
      appMode = APP_MENU_MODE;
      payload.disable();
      int gameResult = 0;
      oled.clear();
      oled.println("-----------------");
      oled.println("-- GAME OVER   --");
      if ( encButton.isLongPressed() ){
        oled.println("-- CANCELLED --");
        gameResult = CANCELLED;
      }
      else if ( game.isOver() ){
        oled.println("- DEFEND WIN   -");
        gameResult = DEFEND_WIN;
      }
      else{
        oled.println("- ATTACK WIN  -");
        gameResult = ATTACK_WIN;
      }
      oled.print("REM TIME:");
      oled.println(game.getSecondsRemaining());
      gameOverDisplay(gameResult); 
      oled.clear();  
      nav.refresh();         
    }
  }
  else if ( appMode == APP_MENU_MODE){
    encoderDriver.update();          
    FastLED.delay(100);    
  }
  //
}

void readGameSettings(){
  EEPROM.get(EEPROMStartAddress, tmpGameOptions);
  if (tmpGameOptions.firmwareVersion != gameOptions.firmwareVersion) {
    // Overwrite with defaults
    Serial.println("Firmware value does not match so writing defaults!");
    writeGameSettings();
  }
  else {
    // Do not overwrite leave values alone
    Serial.println("Firmware values are lining up so reading EEPROM values into settings");
    gameOptions = tmpGameOptions;
  }  
}

void writeGameSettings(){
    stopTimers();
    Serial.println("Writing current settings to EEPROM");
    EEPROM.put(EEPROMStartAddress, gameOptions); // Store the object to EEPROM
    Serial.println("Committing to EEPROM");
    EEPROM.commit();
    startTimers();
}