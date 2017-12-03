
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <EEPROM.h>
#include <FastLED.h>
#include "util.h"
#include "battlepoint.h"
#include "LcdKeypad.h"
#include "MenuData.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "sound.h"
#include "lcd_helper.h"

////////////////////////////
//PIN DEFINITIONS and hardware setup
////////////////////////////
#define RED_BTN_PIN 4
#define BLU_BTN_PIN 9
#define CONTROL_POINT_LED_PIN 5
#define TIMER_LED_PIN 10
#define SOFTSERIAL_TX 2
#define SOFTSERIAL_RX 3
#define BLU_LED_PIN 9
#define RED_LED_PIN 10
#define RANDOM_SEED_PIN 5
#define VOLTAGE_CHECK_PIN 7
#define BRIGHTNESS_PIN 6
#define CONTROL_POINT_LED_CNT 41      //( 0:20: owner, 21-40: capture )
#define TIMER_LED_CNT 40   //(0-30: timer1, 31-60: timer2 )
#define BLU_SWITCH_LED_CNT 1 
#define RED_SWITCH_LED_CNT 1
#define OLED_I2C_ADDRESS 0x3C

////////App Constants
#define BATTERY_LOW_VOLTAGE 6.2F
#define DFPLAYER_BAUD 9600
#define SERIALBAUD 57600
#define BRIGHTNESS 50
#define LOOP_DELAY 100
#define DFPLAYER_VOLUME 10
#define SPLASH_MILLIS 1000
#define GAME_END_MILLIS 5000
#define BP_VERSION 243
#define DEFAULT_CAPTURE_SECONDS 20
#define DEFAULT_GAME_TIME 60
#define DEFAULT_BUTTON_SECONDS 5
#define DEFAULT_START_DELAY 5
#define DEFAULT_MODE 0
#define CONFIG_START 0
////////////////////////

CRGB control_point_leds [CONTROL_POINT_LED_CNT];
CRGB timer_leds [TIMER_LED_CNT];
CRGB blu_switch_leds [BLU_SWITCH_LED_CNT];
CRGB red_switch_leds [RED_SWITCH_LED_CNT];

enum AppModeValues
{
  APP_GAME_RUNNING,
  APP_MENU_MODE,
  APP_PROCESS_MENU_CMD
};

SoftwareSerial mySoftwareSerial(SOFTSERIAL_RX, SOFTSERIAL_TX); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

byte appMode = APP_MENU_MODE;
MenuManager Menu(bpMenu_Root, menuCount(bpMenu_Root));

byte btn;
uint8_t volume = DFPLAYER_VOLUME;
uint8_t brightness = BRIGHTNESS;
void refreshMenuDisplay (byte refreshMode);
byte getNavAction();


LedMeter captureMeter = LedMeter(control_point_leds,0,21);
LedMeter ownerMeter = LedMeter(control_point_leds,21,20);
LedMeter timer1Meter = LedMeter(timer_leds,0,20);
LedMeter timer2Meter = LedMeter(timer_leds,20,20);
LedMeter bluSwitch = LedMeter(blu_switch_leds,0,1);
LedMeter redSwitch = LedMeter(red_switch_leds,0,1);

GameOptions game_options = GameOptions();


DeadbandTimer dt1 = DeadbandTimer();
DeadbandTimer dt2 = DeadbandTimer();
DeadbandTimer dt3 = DeadbandTimer();
DeadbandTimer dt4 = DeadbandTimer();
DeadbandTimer dt5 = DeadbandTimer();
EventManager eventManager = EventManager(&myDFPlayer,&dt1,&dt2,&dt3,&dt4,&dt5);

//proximity(button threshold seconds)
Proximity proximity(&game_options,RED_BTN_PIN,BLU_BTN_PIN);
ControlPoint controlPoint = ControlPoint(&captureMeter,&ownerMeter,&eventManager,&bluSwitch,&redSwitch);
Game game = Game(&controlPoint,&game_options,&eventManager,&proximity,&ownerMeter, &captureMeter, &timer1Meter,&timer2Meter);

SSD1306AsciiWire oled;
//this display basically lets us do fancy stuff
LcdDisplay lcd_display = LcdDisplay(&oled);

void setup() {
    Serial.begin(SERIALBAUD);
/**
  loadSettings();
  
  Wire.begin();                
  oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
  lcd_display.init();
  oled.set400kHz(); 
  oled.setFont(System5x7);
  oled.clear();
 

  mySoftwareSerial.begin(DFPLAYER_BAUD);
  Serial.begin(SERIALBAUD);

  #ifdef BP_DEBUG  
  Serial.println(F("Welcome To BattlePoint v0.1"));
  Serial.println();
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  #endif
  
  if (!myDFPlayer.begin(mySoftwareSerial,false)) {  //Use softwareSerial to communicate with mp3.
 
    #ifdef BP_DEBUG    
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    #endif

    while(true){
      delay(0); 
    }
  }

  #ifdef BP_DEBUG    
  Serial.println(F("DFPlayer Mini online."));  
  #endif

  appMode = APP_MENU_MODE;  
    
  pinMode(RED_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RED_BTN_PIN), red_btn_isr, CHANGE);
  pinMode(BLU_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BLU_BTN_PIN), blu_btn_isr, CHANGE);

  pinMode(BRIGHTNESS_PIN,INPUT);
  FastLED.addLeds<NEOPIXEL, CONTROL_POINT_LED_PIN>(control_point_leds, CONTROL_POINT_LED_CNT);
  FastLED.addLeds<NEOPIXEL, TIMER_LED_PIN>(timer_leds, TIMER_LED_CNT);
  FastLED.addLeds<NEOPIXEL, BLU_LED_PIN>(blu_switch_leds, BLU_SWITCH_LED_CNT);
  FastLED.addLeds<NEOPIXEL, RED_LED_PIN>(red_switch_leds, RED_SWITCH_LED_CNT);
  
  #ifdef BP_DEBUG
  Serial.print("Queuing Background Track...");
  #endif

    //Set volume value. From 0 to 30
  myDFPlayer.volume(volume);
  randomSeed(analogRead(RANDOM_SEED_PIN));
  long r = random(18000,25000);
  //Serial.println("Random Number");
  //Serial.println(r/1000);
  myDFPlayer.loop((int)r/1000); 

  lcd_display.printLine(0,F("BattlePoint v0.1"));
  lcd_display.sprintfLine(6,F("(c)Dcowden,%d"),2017);
  FastLED.delay(SPLASH_MILLIS);
  FastLED.setBrightness(BRIGHTNESS);
  **/
}

void loadSettings(){
  
  SettingsData data = {
     //overall options
     BP_VERSION,
     DFPLAYER_VOLUME,
     BRIGHTNESS,
     
     //game options
     DEFAULT_GAME_TIME,
     DEFAULT_MODE,
     DEFAULT_CAPTURE_SECONDS,
     DEFAULT_BUTTON_SECONDS,
     DEFAULT_START_DELAY
  };

  if ( EEPROM.read(CONFIG_START) == BP_VERSION ){
    for (unsigned int t=0; t<sizeof(data); t++)
      *((char*)&data + t) = EEPROM.read(CONFIG_START + t);    
  }
  
  volume = data.volume;
  brightness = data.brightness;
  game_options.init( data.mode, data.captureSeconds, data.captureButtonThresholdSeconds, data.timeLimitSeconds, data.startDelaySeconds);

}
void red_btn_isr(){
  proximity.redButtonPress();
}
void blu_btn_isr(){
  proximity.bluButtonPress();
}

void loop(){
  Serial.println("Test!!");
  /**
  btn = getButton(); 
  switch (appMode)
  {
    case APP_GAME_RUNNING :
    {
      game.update();
      FastLED.show();
      update_lcd_game_status();
      if ( btn == BUTTON_RIGHT_PRESSED || btn == BUTTON_LEFT_PRESSED 
              || btn == BUTTON_SELECT_PRESSED ||btn ==  BUTTON_UP_PRESSED || btn == BUTTON_DOWN_PRESSED ){
         //cancel game
         game.end();
         lcd_display.printLine(1,F("Game Cancelled"));
         lcd_display.clearLine(2);
         FastLED.delay(GAME_END_MILLIS);        
         myDFPlayer.loop(SND_SOUNDS_0036_GAMESTARTUP15);        
         appMode = APP_MENU_MODE;                  
      }
      if (! game.isRunning() && game.isOver() ){
        //wait a little bit for cp sound to end
        FastLED.delay(200);        
        char tmp_team[4];
        print_team_text(tmp_team,game.winner());
        lcd_display.printLine(1,F("Game Over"));
        lcd_display.sprintfLine(2,F("Winner is %s"),tmp_team);
        
        //delay so we can see who won
        FastLED.delay(GAME_END_MILLIS);        
        myDFPlayer.loop(SND_SOUNDS_0036_GAMESTARTUP15);        
        appMode = APP_MENU_MODE;
      }
      break;
    }
    case APP_MENU_MODE :
    {
      refreshMenuDisplay(REFRESH_DESCEND);
      byte menuMode = Menu.handleNavigation(getNavAction, refreshMenuDisplay);
      if (menuMode == MENU_INVOKE_ITEM)
      {
        appMode = APP_PROCESS_MENU_CMD;
      }
      break;
    }
    case APP_PROCESS_MENU_CMD :
    {
      byte processingComplete = processMenuCommand(Menu.getCurrentItemCmdId());
      if (processingComplete)
      {
        appMode = APP_MENU_MODE;
      }
      break;
    }
  }
  display_voltage();
  myDFPlayer.volume(volume);
  //uint8_t brightness = get_brightness_knob();  
  FastLED.setBrightness(brightness);
  FastLED.delay(LOOP_DELAY);  

  **/
}

int get_brightness_knob(){
  int brightness = analogRead(BRIGHTNESS_PIN);
  return brightness/4;
}

void display_voltage(){
  float voltage = check_supply_voltage();
  char str_temp[6];
  dtostrf(voltage, 4, 2, str_temp);
  if ( voltage < BATTERY_LOW_VOLTAGE ){            
    lcd_display.sprintfLine(5,F("LOW BATT: V: %s"),str_temp);        
  }
  else{
    lcd_display.sprintfLine(5,F("+V: %s"),str_temp); 
  }  
}

float check_supply_voltage(){
  int voltage_ad = analogRead(VOLTAGE_CHECK_PIN);
  float voltage = voltage_ad * ( 5.00 / 1023.00 ) * 2.0;
  return voltage;
}
void update_lcd_game_status(){

  if ( game_options.getMode() == GAME_MODE_KOTH ){
    lcd_display.sprintfLine(1,
         F("R: %d  B: %d"),
         game.getRedTimeLeftSeconds(),
         game.getBluTimeLeftSeconds() );
  }
  else if ( game_options.getMode() == GAME_MODE_AD ){
    lcd_display.sprintfLine(1,
       F("T: %d"),game.getRedTimeLeftSeconds() );
  }
  else if ( game_options.getMode() == GAME_MODE_CP ){
    lcd_display.sprintfLine(1,
         F("R: %d  B: %d"),
         game.getRedMillis(),
         game.getBluMillis() );      
  }
  lcd_display.sprintfLine(2,
    F("CP:%c OWN:%c ON:%c"),
    team_text_char(controlPoint.getCapturing()),
    team_text_char(controlPoint.getOwner()),
    team_text_char(controlPoint.getOn()) );
}

//----------------------------------------------------------------------
// Addition or removal of menu items in MenuData.h will require this method
// to be modified accordingly. 
byte processMenuCommand(byte cmdId)
{
  byte complete = false;  // set to true when menu command processing complete.
  if (btn == BUTTON_SELECT_PRESSED)
  {
    complete = true;
  }

  switch (cmdId){
    case mnuCmdmode: {
      handle_change_mode();
      break;
    }
    case mnuCmdcap: {
      handle_change_capture();
      break;
    }
    case mnuCmdtime: {
      handle_change_timelimit();
      break;
    }
    case mnuCmdstartdelay: {
      handle_change_startdelay();
      break;
    }
    case mnuCmdtapdur: {
      handle_change_buttonduration();     
      break;
    }
    case mnuCmdVolume: {
      handle_change_volume();     
      break;
    }    
    case mnuCmdBrightness: {
      handle_change_brightness();     
      break;
    }     
    case mnuCmdSaveSettings: {
      handle_save_settings();
      complete=true;     
      break;
    }    
    case mnuCmdstart: {
      handle_start_game();
      appMode = APP_GAME_RUNNING;
      break;
    }
    default:{
      break;
    }
  
  }
  return complete;
}

void handle_change_mode(){
  int r = get_int_delta_from_btn(btn,1,1);
  game_options.setMode(game_options.getMode()+r);      
  
  char mode_text[6];
  print_game_mode_text(mode_text, game_options.getMode()); 
  lcd_display.sprintfLine(2,F("Mode: %s"),mode_text);

}
void handle_voltage_check(){
  float voltage = check_supply_voltage();
  char str_temp[6];
  dtostrf(voltage, 4, 2, str_temp);
  lcd_display.sprintfLine(2,F("+V: %s"),str_temp);

}
void handle_change_capture(){
  int r = get_int_delta_from_btn(btn,1,10);
  game_options.setCaptureSeconds(game_options.getCaptureSeconds()+r);
  lcd_display.sprintfLine(2,F("Cap: %d"),game_options.getCaptureSeconds());
}
void handle_change_timelimit(){
  int r = get_int_delta_from_btn(btn,5,30);      
  game_options.setTimeLimitSeconds(game_options.getTimeLimitSeconds()+r);
  lcd_display.sprintfLine(2,F("Time: %d"),game_options.getTimeLimitSeconds());
}
void handle_save_settings(){
  save_settings();
  lcd_display.printLine(2,F("Saved.") );

}

void handle_change_volume(){
  int r = get_int_delta_from_btn(btn,5,10);
  if ( volume > 100 ) volume = 100;
  if ( volume < 0 ) volume = 0;      
  volume = volume +r;
  lcd_display.sprintfLine(2,F("Vol: %d"),volume);
}
void handle_change_brightness(){
  int r = get_int_delta_from_btn(btn,5,10);
  if ( brightness > 100 ) brightness = 100;
  if ( brightness < 0 ) brightness = 0;      
  brightness = brightness +r;
  lcd_display.sprintfLine(2,F("Btness: %d"),brightness);
}

void handle_change_startdelay(){
  int r = get_int_delta_from_btn(btn,1,10);      
  game_options.setStartDelaySeconds(game_options.getStartDelaySeconds()+r);
  lcd_display.sprintfLine(2,F("Delay: %d"),game_options.getStartDelaySeconds());
 
}
void handle_change_buttonduration(){
  int r = get_int_delta_from_btn(btn,1,1);          
  game_options.setCaptureButtonThresholdSeconds(game_options.getCaptureButtonThresholdSeconds()+r);
  lcd_display.sprintfLine(2,F("Tap: %d"),game_options.getCaptureButtonThresholdSeconds());

}
void handle_start_game(){
  //TODO: this really should move into the game itself
  int start_delay = game_options.getStartDelaySeconds();
  timer1Meter.init();
  timer2Meter.init();
  ownerMeter.init();
  captureMeter.init();
  
  ownerMeter.fgColor(CRGB::Yellow);
  captureMeter.fgColor(CRGB::Yellow);
  timer1Meter.fgColor(CRGB::Yellow);
  timer2Meter.fgColor(CRGB::Yellow);
  timer1Meter.setMaxValue(start_delay);
  ownerMeter.setMaxValue(10);
  captureMeter.setMaxValue(10);
  ownerMeter.setToMax();
  captureMeter.setToMax();
  timer2Meter.setMaxValue(start_delay);
  lcd_display.printLine(1,F("Starting Game"));
  eventManager.starting_game();
  for ( int i=start_delay;i>0;i--){
    lcd_display.sprintfLine(2,F("Starts in: %d s"),i );
    eventManager.starts_in_seconds(i);
    timer1Meter.setValue(i);
    timer2Meter.setValue(i);
    FastLED.show();
    FastLED.delay(1000);
  }      
  game.start();  
}

void save_settings(){
   
   SettingsData sd = {
     BP_VERSION,
     //overall options
     volume,
     brightness,
     
     //game options
     game_options.getTimeLimitSeconds(),
     game_options.getMode(),
     game_options.getCaptureSeconds(),
     game_options.getCaptureButtonThresholdSeconds(),
     game_options.getStartDelaySeconds()     
   };
    for (unsigned int t=0; t<sizeof(sd); t++)
        EEPROM.write(CONFIG_START + t, *((char*)&sd + t));
}

int get_int_delta_from_btn(byte btn,uint8_t shortPressIncrement, uint8_t longPressIncrement){
  int r = 0;
  switch ( btn ){
    case BUTTON_UP_PRESSED:{
      r= shortPressIncrement;
      break;}
    case BUTTON_DOWN_PRESSED:{
      r= -shortPressIncrement;
      break;}
    case BUTTON_UP_LONG_PRESSED:{
      r=longPressIncrement;
      break;}
    case BUTTON_DOWN_LONG_PRESSED:{
      r=-longPressIncrement;
      break;}
  }
  return r;
}

//----------------------------------------------------------------------
// Callback to convert button press to navigation action.
byte getNavAction()
{
  //TODO: dont need most of this
  byte navAction = 0;
  byte currentItemHasChildren = Menu.currentItemHasChildren();
  
  if (btn == BUTTON_UP_PRESSED || btn == BUTTON_UP_LONG_PRESSED) navAction = MENU_ITEM_NEXT;
  else if (btn == BUTTON_DOWN_PRESSED || btn == BUTTON_DOWN_LONG_PRESSED) navAction = MENU_ITEM_PREV;
  else if (btn == BUTTON_SELECT_PRESSED || (btn == BUTTON_RIGHT_PRESSED && currentItemHasChildren)) navAction = MENU_ITEM_SELECT;
  //else if (btn == BUTTON_LEFT_PRESSED) navAction = MENU_BACK;
  return navAction;
}

void refreshMenuDisplay (byte refreshMode)
{
  char nameBuf[LCD_COLS+1];
  Menu.getCurrentItemName(nameBuf);
  lcd_display.printLine(1,nameBuf);
  lcd_display.clearLine(2);
}

