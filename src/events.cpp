#include "battlepoint.h"
#include "sound.h"
#include "DFRobotDFPlayerMini.h"
#define DEFAULT_CONTEST_ALERT_INTERVAL 20000
#define START_END_WINDOW_INTERVAL 900
#define OVERTIME_ALERT_INTERVAL 5000
#define LARGE_TIME 15000
#define DEFAULT_CAPTURE_SECONDS 20
#define DEFAULT_GAME_TIME 60
#define DEFAULT_BUTTON_SECONDS 5
#define DEFAULT_CAPTURE_SECONDS 10
#define DEFAULT_START_DELAY 5

EventManager::EventManager(DFRobotDFPlayerMini* player,
          DeadbandTimer* captureTimer,
          DeadbandTimer* contestTimer,
          DeadbandTimer* overtimeTimer,
          DeadbandTimer* startTimeTimer,
          DeadbandTimer* endTimeTimer
    ){
      _player = player;
      _captureTimer= captureTimer;
      _contestTimer= contestTimer;
      _overtimeTimer=overtimeTimer ;
      _startTimeTimer=startTimeTimer;
      _endTimeTimer=endTimeTimer;      
}
void EventManager::init(int cpAlertIntervalSeconds){
  _cpAlertIntervalMilliSeconds = (long)(cpAlertIntervalSeconds*1000);
}
void EventManager::control_point_being_captured(uint8_t team){
    if ( _captureTimer->notInDeadband(_cpAlertIntervalMilliSeconds) ){
      #ifdef BP_DEBUG
      Serial.print(F("Control Point Being Captured."));
      #endif
      _player->play(SND_SOUNDS_0014_ANNOUNCER_LAST_FLAG);
    }
}
void EventManager::control_point_contested(){
    if (_contestTimer->notInDeadband(DEFAULT_CONTEST_ALERT_INTERVAL) ){
        #ifdef BP_DEBUG
        Serial.println(F("Control Point is being Contested!"));
        #endif
        _player->play(SND_SOUNDS_0002_ANNOUNCER_ALERT_CENTER_CONTROL_BEING_CONTESTED);
    }
}
void EventManager::control_point_captured(uint8_t team){
    _player->play(SND_SOUNDS_0025_ANNOUNCER_WE_CAPTURED_CONTROL);
    #ifdef BP_DEBUG
    Serial.println(F("Control Point Captured."));
    #endif
}
void EventManager::cancelled(){
  _player->play(SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10);  
}
void EventManager::starting_game(){
  _player->play(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED);
  delay(500);
}
void EventManager::game_started(){
    _player->play(SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4);
    #ifdef BP_DEBUG
    Serial.print(F("Game Started"));
    #endif
}
void EventManager::victory(uint8_t team){
    #ifdef BP_DEBUG
    Serial.println("Victory Sound.");
    #endif
    FastLED.delay(200);
    _player->play(SND_SOUNDS_0023_ANNOUNCER_VICTORY);
}
void EventManager::overtime(){
  if (_overtimeTimer->notInDeadband(_cpAlertIntervalMilliSeconds) ){
    _player->play(SND_SOUNDS_0017_ANNOUNCER_OVERTIME2);
    #ifdef BP_DEBUG
    Serial.print(F("Overtime"));
    #endif
  }
}
void EventManager::starts_in_seconds(int secs){

  if ( _startTimeTimer->notInDeadband(START_END_WINDOW_INTERVAL) ){
    #ifdef BP_DEBUG
    Serial.print("Game Starts in ");
    Serial.print(secs);
    Serial.println(" seconds!");
    #endif

    switch ( secs ){
       case 30:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_30SEC);
          break;
       }
       case 20:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_20SEC);
          break;
       }
       case 10:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_10SEC);
          break;
       }
       case 5:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_5SEC);
          break;
       }
       case 4:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_4SEC);
          break;
       }
       case 3:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_3SEC);
          break;
       }
       case 2:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_2SEC);
          FastLED.delay(100);
          break;
       }
       case 1:{
          _player->play(SND_SOUNDS_ANNOUNCER_BEGINS_1SEC);
          FastLED.delay(100);
          break;
       }
    }

  }
}


void EventManager::ends_in_seconds(int secs){
    #ifdef BP_DEBUG
    Serial.print(F("Ends in "));
    Serial.print(secs);
    Serial.print(F(" seconds!"));
    #endif  
  if ( _endTimeTimer->notInDeadband(START_END_WINDOW_INTERVAL) ){
    
    switch ( secs ){
       case 120:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_2MIN);
          break;
       }
       case 60:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_60SEC);
          break;
       }
       case 30:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_30SEC);
          break;
       }
       case 20:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_20SEC);
          break;
       }
       case 10:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_10SEC);
          break;
       }
       case 5:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_5SEC);
          break;
       }
       case 4:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_4SEC);
          break;
       }
       case 3:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_3SEC);
          break;
       }
       case 2:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_2SEC);
          break;
       }
       case 1:{
          _player->play(SND_SOUNDS_ANNOUNCER_ENDS_1SEC);
          break;
       }
    }
  }
}
Proximity::Proximity(GameOptions* gameOptions,uint8_t redButtonPin, uint8_t bluButtonPin){
   last_btn_red_change_millis = -1;
   last_btn_blu_change_millis = -1;
   _redButtonPin = redButtonPin;
   _bluButtonPin = bluButtonPin;
   _gameOptions = gameOptions;
};
boolean Proximity::isRedClose(){
  uint8_t  btn = digitalRead(_redButtonPin);
  if ( btn == 0 ){
     return true;
  }
  return last_btn_red_change_millis > 0 &&
          (millis() - last_btn_red_change_millis) < (_gameOptions->getCaptureButtonThresholdSeconds() *1000);
};
boolean Proximity::isBluClose(){
  uint8_t  btn = digitalRead(_bluButtonPin);
  if ( btn == 0 ){
     return true;
  }  
  return last_btn_blu_change_millis > 0 &&
         (millis() - last_btn_blu_change_millis) < (_gameOptions->getCaptureButtonThresholdSeconds() *1000);

};

void Proximity::debugStatus(){
  #ifdef BP_DEBUG
  Serial.print(F("CaptureThreshold="));
  Serial.println(_gameOptions->getCaptureButtonThresholdSeconds() *1000);
  Serial.print(F("Red Pressed"));
  Serial.println((last_btn_red_change_millis));
  Serial.print(F("Blu Pressed"));
  Serial.println((last_btn_blu_change_millis));
  Serial.print(F("Blue Is Close="));
  Serial.print(isBluClose());
  Serial.print(F(", Red Is Close="));
  Serial.println(isRedClose());
  #endif
};
void Proximity::redButtonPress(){
    last_btn_red_change_millis = millis();
};

void Proximity::bluButtonPress(){
    last_btn_blu_change_millis = millis();
};

/** Stub implementation. Later might get options via user actions, network, etc **/
GameOptions::GameOptions( ){
};
void GameOptions::init(uint8_t mode, uint8_t captureSeconds, uint8_t buttonThresholdSeconds, uint8_t timeLimitSeconds, uint8_t startDelaySeconds){
     _timeLimitSeconds = timeLimitSeconds;
     _mode = mode;
     _captureSeconds=captureSeconds;
     _captureButtonThresholdSeconds=buttonThresholdSeconds;
     _startDelaySeconds=startDelaySeconds;  
}

uint8_t GameOptions::getStartDelaySeconds() { return _startDelaySeconds; };
int GameOptions::getTimeLimitSeconds(){  return _timeLimitSeconds; };
long GameOptions::getTimeLimitMilliSeconds(){  return ((long)_timeLimitSeconds)*1000; };

uint8_t GameOptions::getCaptureButtonThresholdSeconds(){ return _captureButtonThresholdSeconds; };
uint8_t GameOptions::getMode(){  return _mode; };
uint8_t GameOptions::getCaptureSeconds(){ return _captureSeconds; };

void GameOptions::setStartDelaySeconds(uint8_t startDelaySeconds) {
  if ( startDelaySeconds > 0  ){
      _startDelaySeconds = startDelaySeconds;
  }
}

void GameOptions::setTimeLimitSeconds(int timeLimitSeconds) {
  if ( timeLimitSeconds > 0 && timeLimitSeconds > _captureSeconds ){
      _timeLimitSeconds = timeLimitSeconds;
  }
}
void GameOptions::setCaptureButtonThresholdSeconds(uint8_t captureButtonThresholdSeconds) {
  if ( captureButtonThresholdSeconds > 0 && captureButtonThresholdSeconds < _captureSeconds ){
      _captureButtonThresholdSeconds = captureButtonThresholdSeconds;
  }
}
void GameOptions::setMode(uint8_t mode){
  if ( mode == 0 || mode == 1 || mode == 2 ){
     _mode = mode;
  }
}
void GameOptions::setCaptureSeconds(uint8_t captureSeconds) {
  if ( captureSeconds > 0 && captureSeconds < _timeLimitSeconds ){
    _captureSeconds = captureSeconds;
  }
}
