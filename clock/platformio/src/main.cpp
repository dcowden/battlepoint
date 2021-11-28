#include <Arduino.h>
#include <ArduinoLog.h>
#include <U8g2lib.h>
#include "Ticker.h"
#include "EncoderMenuDriver.h"
#include <display.h>
#include <pins.h>
#include <math.h>
#include <settings.h>
#include <ClickEncoder.h>
#include <menu.h>
#include <menuIO/keyIn.h>
#include <menuIO/u8g2Out.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIO.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialIn.h>
#include <GameClock.h>
#include "ServoGameClock.h"
#include "SevenSegmentMap.h"

#define BATTLEPOINT_VERSION "1.0.3"
#define BP_MENU "BP v1.0.3"

#define MENU_MAX_DEPTH 4
#define OFFSET_X 0
#define OFFSET_Y 0
#define DISPLAY_UPDATE_INTERVAL_MS 100
#define VERTICAL_LED_SIZE 16
#define HORIONTAL_LED_SIZE 12

#define START_DELAY_MIN 1
#define START_DELAY_MAX 120
#define START_DELAY_BIG_STEP_SIZE 10
#define START_DELAY_LITTLE_STEP_SIZE 1

#define GAME_TIME_MIN 10
#define GAME_TIME_MAX 1000
#define GAME_TIME_BIG_STEP_SIZE 10
#define GAME_TIME_LITTLE_STEP_SIZE 1

#define POST_INTERVAL_MS 80
#define TIMER_INTERVAL_MICROSECONDS 1000
#define HARDWARE_INFO_UPDATE_INTERVAL_MS 1000
#define GAME_CLOCK_UPDATE_INTERVAL_MS 1000
#define ENCODER_UPDATE_INTERVAL_MS 5

//the different types of games we can play
ClockSettings clockSettings;
GameClockState clockState;


typedef enum {
    PROGRAM_MODE_GAME=0,
    PROGRAM_MODE_MENU = 1
} ProgramMode;

int programMode = ProgramMode::PROGRAM_MODE_MENU;

// ESP32 timer thanks to: http://www.iotsharing.com/2017/06/how-to-use-interrupt-timer-in-arduino-esp32.html
// and: https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *timer = NULL;

HardwareInfo hardwareInfo;

//prototypes
void IRAM_ATTR onTimer();
void stopGameAndReturnToMenu();
void start();
void updateGameClockLocal();
void updateEncoder();

//from example here: https://github.com/neu-rah/ArduinoMenu/blob/master/examples/ESP32/ClickEncoderTFT/ClickEncoderTFT.ino
ClickEncoder clickEncoder = ClickEncoder(Pins::ENC_DOWN, Pins::ENC_UP, Pins::ENC_BUTTON, 2,true);

double getBatteryVoltage(){
  int r = analogRead(Pins::VBATT);
  return (double)r / 1575.0 * 2.0; //ADC_11db = 1v per 1575 count
}

void updateHardwareInfo(){
  timerAlarmDisable(timer);
  hardwareInfo.vBatt = getBatteryVoltage();
  timerAlarmEnable(timer);  
}

void updateDisplayLocal(){
  if ( programMode == PROGRAM_MODE_GAME){
    updateDisplay(&clockState,&hardwareInfo);
  }
}

void updateGameClockLocal(){
  game_clock_update(&clockState,millis());
  ClockColor cc = game_clock_color_for_state(&clockState);
  servo_clock_update_time(clockState.time_to_display_secs, cc);
  if ( clockState.clockState == ClockState::OVER){
    stopGameAndReturnToMenu();
  }
}

Ticker encoderUpdateTimer(updateEncoder,ENCODER_UPDATE_INTERVAL_MS);
Ticker updateOledDisplayTimer(updateDisplayLocal,DISPLAY_UPDATE_INTERVAL_MS);
Ticker hardwareUpdateDataTimer(updateHardwareInfo, HARDWARE_INFO_UPDATE_INTERVAL_MS);
Ticker gameClockUpdateTimer(updateGameClockLocal, GAME_CLOCK_UPDATE_INTERVAL_MS);

/*
void setupEncoder(){
  clickEncoder.setAccelerationEnabled(true);
  clickEncoder.setDoubleClickEnabled(true); 

  //ESP32 timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_INTERVAL_MICROSECONDS, true); //units are microseconds
  timerAlarmEnable(timer);
}*/

void updateEncoder(){
  clickEncoder.service();
}

void loadSettings(){
  //timerAlarmDisable(timer);
  loadSetting(&clockSettings);
  //timerAlarmEnable(timer);
}

void saveSettings(){
  //timerAlarmDisable(timer);
  saveSetting(&clockSettings);
  //timerAlarmEnable(timer);
}

Menu::result menuaction_loadSavedSettings(){
  loadSettings();
  return Menu::proceed;
}

//FIELD(var.name, title, units, min., max., step size,fine step size, action, events mask, styles)
MENU(mainMenu, BP_MENU, menuaction_loadSavedSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(clockSettings.start_delay_secs,"Start Delay","",START_DELAY_MIN,START_DELAY_MAX,START_DELAY_BIG_STEP_SIZE,START_DELAY_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(clockSettings.game_secs,"Game Time","s",GAME_TIME_MIN,GAME_TIME_MAX,GAME_TIME_BIG_STEP_SIZE,GAME_TIME_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)            
    ,OP("Start",start, Menu::enterEvent)
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

void start(){  
  Log.noticeln("Starting Game Clock");
  saveSettings();
  oled.clear();
  nav.idleOn();
  game_clock_configure(&clockState,clockSettings.start_delay_secs,clockSettings.game_secs);
  game_clock_start(&clockState,millis());
  programMode = PROGRAM_MODE_GAME;
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

void POST(){
  Log.noticeln("POST...");
  int DELAY_MS = 1000;
  servo_clock_blank();
  delay(DELAY_MS);
  servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_8,ClockColor::BLUE);
  delay(DELAY_MS);
  servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_8,ClockColor::RED);
  delay(DELAY_MS);
  servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_8,ClockColor::YELLOW);
  delay(DELAY_MS);
  servo_clock_blank();
  Log.noticeln("POST COMPLETE");
}


void setup() {
  setCpuFrequencyMhz(240);
  hardwareInfo.version = BATTLEPOINT_VERSION;
  Serial.begin(115200);
  Serial.setTimeout(500);
  //setupEncoder();
  initSettings();
  Log.begin(LOG_LEVEL_SILENT, &Serial, true);
  Log.warning("Starting...");
  initDisplay();
  displayWelcomeBanner(hardwareInfo.version);
  Menu::options->invertFieldKeys = false;
  Log.warningln("Complete.");
  oled.setFont(u8g2_font_7x13_mf);
  servo_clock_init();  
  POST();
  updateOledDisplayTimer.start();
  gameClockUpdateTimer.start();
  hardwareUpdateDataTimer.start();
  //loadSettings();

  servo_clock_blank();
  //temporary
  start();
}


void loop() {  
  clickEncoder.service();  
  hardwareUpdateDataTimer.update();
  if ( programMode == PROGRAM_MODE_GAME ){

      gameClockUpdateTimer.update();
      updateOledDisplayTimer.update();
      
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
//void IRAM_ATTR onTimer()
//{  
  //clickEncoder.service();  
//}