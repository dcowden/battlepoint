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
//#include "ServoGameClock.h"
//#include "SevenSegmentMap.h"
#include <FastLED.h>
#include "WiFi.h"
#include <AccelStepper.h>
#include <Wire.h>
#include <sound.h>

#define BATTLEPOINT_VERSION "1.1.0"
#define BP_MENU "BP v1.1.0"

#define MENU_MAX_DEPTH 4
#define OFFSET_X 0
#define OFFSET_Y 0
#define DISPLAY_UPDATE_INTERVAL_MS 100
#define POINTER_LED_SIZE 50


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
#define POSITION_INTERVAL_MS 1000
#define ENCODER_UPDATE_INTERVAL_MS 1

#define DIAL_STEPPER_STEPS_PER_REV 4096.0
#define DIAL_STEPPER_RATIO 1.5
#define DIAL_MINUTES 20.0
#define SECONDS_PER_MINUTE 60.0
#define DIAL_HOMING_MAX_STEPS 4100
#define DIAL_BACKLASH_STEPS 40 //have to add this many steps when we reverse direction
#define DIAL_MAX_SPEED 700
#define CLOCK_SPEED -7 //nomially its 5 steps per second, but this makes sure we stay up to date.
#define DIAL_HOME_SPEED -700

//the different types of games we can play
ClockSettings clockSettings;
GameClockState clockState;
CRGB pointerLeds[POINTER_LED_SIZE];
AccelStepper dialStepper (AccelStepper::HALF4WIRE, DIAL_1, DIAL_3, DIAL_2, DIAL_4,true);


typedef enum {
    PROGRAM_MODE_GAME=0,
    PROGRAM_MODE_MENU = 1
} ProgramMode;

int programMode = ProgramMode::PROGRAM_MODE_MENU;
int manualMenuDigit  = 0;
int manualClockSeconds = 0;
int manualClockAngle = 0;
int soundId = 0;
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
void updateDialPosition();
void printDialPosition();
void refreshDisplay();


int getStepPositionForSeconds(int sec){
  const float SECONDS_PER_REV = DIAL_MINUTES * SECONDS_PER_MINUTE;
  float r = DIAL_STEPPER_STEPS_PER_REV /  SECONDS_PER_REV * sec * DIAL_STEPPER_RATIO;
  return (int)r;
}

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
void updateDialLeds( CRGB color){
  for ( int i=0;i<POINTER_LED_SIZE;i++){
    pointerLeds[i] = color;    
  }
  FastLED.show();
}

void updateDialPosition(int position_steps, int nominal_speed){
  int spd = nominal_speed;
  if ( position_steps < dialStepper.currentPosition()){
    spd = -spd;
  }
  dialStepper.moveTo(position_steps);
  dialStepper.setSpeed(spd);
  //Serial.print("SetPosition=");Serial.println(position_steps);
  //Serial.print("Speed=");Serial.println(dialStepper.speed());  
}

CRGB getLedColorForClockColor(ClockColor color){
    if ( color == ClockColor::YELLOW){
        return CRGB::Yellow;
    }
    else if ( color == ClockColor::RED){
        return CRGB::Red;
    }
    else if ( color == ClockColor::BLUE){
        return CRGB::Blue;
    }
    else if ( color == ClockColor::GREEN){
        return CRGB::Green;
    }    
    else{
      return CRGB::Black;
    }
}


void updateDialLeds( ClockColor color ){
  CRGB dialColor = getLedColorForClockColor(color);
  updateDialLeds(dialColor);
}

void updateGameClockLocal(){
  //TODO: move this logic to different class
  //assume that we started positoined at delay time plus game time, we're stricktly counting down


  game_clock_update(&clockState,millis());
  ClockColor cc = game_clock_color_for_state(&clockState);
  //servo_clock_update_time(clockState.time_to_display_secs, cc);

  int secs_to_display = 0;
  if ( clockState.secs_till_start > 0){
    secs_to_display = clockState.game_duration_secs + clockState.secs_till_start;
  }
  else{
    secs_to_display = clockState.game_remaining_secs;
  }
  
  int position = getStepPositionForSeconds(secs_to_display);
  updateDialPosition(position,CLOCK_SPEED);

  updateDialLeds(cc);
  if ( clockState.clockState == ClockState::OVER){
    stopGameAndReturnToMenu();
  }
}

Ticker updateOledDisplayTimer(updateDisplayLocal,DISPLAY_UPDATE_INTERVAL_MS);
Ticker hardwareUpdateDataTimer(updateHardwareInfo, HARDWARE_INFO_UPDATE_INTERVAL_MS);
Ticker gameClockUpdateTimer(updateGameClockLocal, GAME_CLOCK_UPDATE_INTERVAL_MS);
Ticker dialPositionTimer(printDialPosition, POSITION_INTERVAL_MS);

void setupLEDs(){
  FastLED.addLeds<NEOPIXEL, Pins::LED_TOP>(pointerLeds, POINTER_LED_SIZE);
}

void setupDialStepper(){
  dialStepper.setMaxSpeed(1000);
  dialStepper.setAcceleration(1200);
}

void setupEncoder(){
  clickEncoder.setAccelerationEnabled(true);
  clickEncoder.setDoubleClickEnabled(true); 

  //ESP32 timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIMER_INTERVAL_MICROSECONDS, true); //units are microseconds
  timerAlarmEnable(timer);
}

void homeDial(){
  updateDialLeds(CRGB::Aqua);
  Log.infoln("Beginning Homing...");
  dialStepper.setSpeed(DIAL_HOME_SPEED);
  while( 1){
      dialStepper.runSpeed();
      int home = digitalRead(Pins::DIAL_INDEX);      
      if ( home == 0){
         break;
      }
  }
  dialStepper.setCurrentPosition(0);
  //take up backlash
  dialStepper.moveTo(DIAL_BACKLASH_STEPS);
  dialStepper.runToPosition();
  dialStepper.setCurrentPosition(0);
  Log.infoln("Dial Homed");
}

void handleChangedDialSeconds(){
  int p = getStepPositionForSeconds(manualClockSeconds);
  updateDialPosition(p, DIAL_MAX_SPEED);
}

void handleChangedDigit(){
  int seconds =  manualMenuDigit * 60;
  int p = getStepPositionForSeconds(seconds);
  updateDialPosition(p,DIAL_MAX_SPEED);    
}
void handleChangedSound(){
  sound_play(soundId);
}
void loadSettings(){
  timerAlarmDisable(timer);
  loadSetting(&clockSettings);
  timerAlarmEnable(timer);
}

void saveSettings(){
  timerAlarmDisable(timer);
  saveSetting(&clockSettings);
  timerAlarmEnable(timer);
}

Menu::result menuaction_loadSavedSettings(){
  loadSettings();
  return Menu::proceed;
}

//FIELD(var.name, title, units, min., max., step size,fine step size, action, events mask, styles)

SELECT(manualMenuDigit,digitMenu,"Minute",handleChangedDigit,Menu::exitEvent  ,Menu::noStyle
      ,VALUE("0",0,Menu::doNothing,Menu::noEvent)
      ,VALUE("1",1,Menu::doNothing,Menu::noEvent)
      ,VALUE("2",2,Menu::doNothing,Menu::noEvent)
      ,VALUE("3",3,Menu::doNothing,Menu::noEvent)
      ,VALUE("4",4,Menu::doNothing,Menu::noEvent)
      ,VALUE("5",5,Menu::doNothing,Menu::noEvent)
      ,VALUE("6",6,Menu::doNothing,Menu::noEvent)
      ,VALUE("7",7,Menu::doNothing,Menu::noEvent)
      ,VALUE("8",8,Menu::doNothing,Menu::noEvent)
      ,VALUE("9",9,Menu::doNothing,Menu::noEvent)
);


//FIELD(var.name, title, units, min., max., step size,fine step size, action, events mask, styles)
MENU(mainMenu, BP_MENU, menuaction_loadSavedSettings, Menu::enterEvent, Menu::wrapStyle
    ,FIELD(clockSettings.start_delay_secs,"Start Delay","",START_DELAY_MIN,START_DELAY_MAX,START_DELAY_BIG_STEP_SIZE,START_DELAY_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(clockSettings.game_secs,"Game Time","s",GAME_TIME_MIN,GAME_TIME_MAX,GAME_TIME_BIG_STEP_SIZE,GAME_TIME_LITTLE_STEP_SIZE,Menu::doNothing,Menu::noEvent,Menu::noStyle)
    ,FIELD(manualClockSeconds,"Clock ","s",0,1200,100,10,handleChangedDialSeconds,Menu::exitEvent,Menu::noStyle)
    ,FIELD(soundId,"PlaySound","",1,120,1,1,handleChangedSound,Menu::exitEvent,Menu::noStyle)
    ,SUBMENU(digitMenu)            
    ,OP("Home",homeDial, Menu::enterEvent)
    ,OP("Start",start, Menu::enterEvent)
);

Menu::serialIn serial(Serial);

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

MENU_INPUTS(in,&serial);

MENU_OUTPUTS(out,MENU_MAX_DEPTH
  ,U8G2_OUT(oled,colors,fontW,fontH,OFFSET_X,OFFSET_Y,{0,0,charWidth,lineHeight})
  //,SERIAL_OUT(Serial)
  ,NONE
);

NAVROOT(nav, mainMenu, MENU_MAX_DEPTH, in, out);

EncoderMenuDriver menuDriver = EncoderMenuDriver(&nav, &clickEncoder);

void start(){  
  Log.traceln("Starting Game Clock");
  saveSettings();
  oled.clear();
  nav.idleOn();
  game_clock_configure(&clockState,clockSettings.start_delay_secs,clockSettings.game_secs);
  game_clock_start(&clockState,millis());
  programMode = PROGRAM_MODE_GAME;

  //at this point we need to zip over to the starting position
  int start_secs = clockState.game_duration_secs + clockState.start_delay_secs;
  int p = getStepPositionForSeconds(start_secs);
  dialStepper.moveTo(p+ DIAL_BACKLASH_STEPS);
  dialStepper.runToPosition();
  //take up backlash
  dialStepper.moveTo(p);
  dialStepper.runToPosition();
  updateDialLeds(CRGB::Yellow);
}

void stopGameAndReturnToMenu(){
  oled.clear();
  nav.idleOff();
  refreshDisplay();  
  dialStepper.stop();
  play_random_startup();
  programMode = PROGRAM_MODE_MENU;
  
}

/**
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
} **/

void POST(){
  Log.noticeln("POST...");
  long DELAY_MS = 500;
  //servo_clock_blank();
  updateDialLeds(CRGB::Red);
  delay(DELAY_MS);
  updateDialLeds(CRGB::Yellow);
  delay(DELAY_MS);
  updateDialLeds(CRGB::Blue);
  delay(DELAY_MS);
  updateDialLeds(CRGB::Green);

  //updateDialLeds(ClockColor::YELLOW);
  //delay(DELAY_MS);
  //updateDialLeds(CRGB::Black);
  //servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_8,ClockColor::BLUE);
  //delay(DELAY_MS);
  //servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_3,ClockColor::RED);
  //delay(DELAY_MS);
  //servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_1,ClockColor::YELLOW);
  
  //servo_clock_blank();
  //delay(DELAY_MS);
 
  Log.noticeln("POST COMPLETE");
}

void setup() {
  setCpuFrequencyMhz(240);
  setupLEDs();
  WiFi.mode(WIFI_OFF);
  Wire.begin(21,22);
  sound_init(Pins::DF_RX, Pins::DF_TX);
  btStop();
  hardwareInfo.version = BATTLEPOINT_VERSION;
  Serial.begin(115200);
  Serial.setTimeout(500);
  pinMode(DIAL_INDEX,INPUT_PULLUP);
  initSettings();
  
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.warning("Starting...");
  initDisplay();
  displayWelcomeBanner(hardwareInfo.version);
  Menu::options->invertFieldKeys = false;
  Log.warningln("Complete.");
  oled.setFont(u8g2_font_7x13_mf);
  //servo_clock_init();  
  POST();
  dialPositionTimer.start();
  updateOledDisplayTimer.start();
  gameClockUpdateTimer.start();
  //hardwareUpdateDataTimer.start();
  setupDialStepper();
  setupEncoder();
  loadSettings();
  homeDial();
  //servo_clock_blank();
  //start();
  //updateDialLeds(CRGB::Black);
  refreshDisplay();
  play_random_startup();
}

void refreshDisplay(){
    oled.setFont(u8g2_font_7x13_mf);
    oled.firstPage();
    do nav.doOutput(); while (oled.nextPage() );  
    Serial.println("Refresh!");
}
void loop() {  

  //hardwareUpdateDataTimer.update();
  if ( programMode == PROGRAM_MODE_GAME ){

      gameClockUpdateTimer.update();
      updateOledDisplayTimer.update();
      
      int b = clickEncoder.getButton();
      if ( b == ClickEncoder::DoubleClicked){
        sound_play(SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10);
        stopGameAndReturnToMenu();
      }
  }
  else if ( programMode == PROGRAM_MODE_MENU){
    menuDriver.update();
    if (menuDriver.dirty ) {
       refreshDisplay();
    }
  }
  else{
      Serial.println("Unknown Program Mode");
  }

  dialStepper.runSpeedToPosition();
  dialPositionTimer.update();
}

void printDialPosition(){
  Serial.print("P=");Serial.print(dialStepper.currentPosition());Serial.print(",W=");Serial.println(dialStepper.speed());
}

// ESP32 timer
void IRAM_ATTR onTimer()
{  
  clickEncoder.service();  
}