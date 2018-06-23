#include <GameAudioManager.h>
#include <Teams.h>
#include <sound.h>

#define DEFAULT_CONTEST_ALERT_INTERVAL 20000
#define START_END_WINDOW_INTERVAL 900
#define OVERTIME_ALERT_INTERVAL 5000

//#define LARGE_TIME 15000
//#define DEFAULT_CAPTURE_SECONDS 20
//#define DEFAULT_GAME_TIME 60
//#define DEFAULT_BUTTON_SECONDS 5
//#define DEFAULT_START_DELAY 5

GameAudioManager::GameAudioManager(DFRobotDFPlayerMini* player,long cpAlertIntervalMilliSeconds){
      _player = player;
      _captureTimer= new CooldownTimer(cpAlertIntervalMilliSeconds);
      _contestTimer= new CooldownTimer(DEFAULT_CONTEST_ALERT_INTERVAL);
      _overtimeTimer=new CooldownTimer(cpAlertIntervalMilliSeconds);
      _startTimeTimer=new CooldownTimer(START_END_WINDOW_INTERVAL);
      _endTimeTimer=new CooldownTimer(START_END_WINDOW_INTERVAL);    
}


void GameAudioManager::control_point_being_captured(Team team){
    if ( _captureTimer->canRun() ){
      #ifdef BP_DEBUG
      Serial.print(F("Control Point Being Captured."));
      #endif
      _player->play(SND_SOUNDS_0014_ANNOUNCER_LAST_FLAG);
    }
}
void GameAudioManager::control_point_contested(){
    if (_contestTimer->canRun() ){
        #ifdef BP_DEBUG
        Serial.println(F("Control Point is being Contested!"));
        #endif
        _player->play(SND_SOUNDS_0002_ANNOUNCER_ALERT_CENTER_CONTROL_BEING_CONTESTED);
    }
}
void GameAudioManager::control_point_captured(Team team){
    _player->play(SND_SOUNDS_0025_ANNOUNCER_WE_CAPTURED_CONTROL);
    #ifdef BP_DEBUG
    Serial.println(F("Control Point Captured."));
    #endif
}
void GameAudioManager::cancelled(){
  _player->play(SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10);  
}
void GameAudioManager::starting_game(){
  _player->play(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED);
  delay(500);
}
void GameAudioManager::game_started(){
    _player->play(SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4);
    #ifdef BP_DEBUG
    Serial.print(F("Game Started"));
    #endif
}
void GameAudioManager::victory(Team team){
    #ifdef BP_DEBUG
    Serial.println("Victory Sound.");
    #endif
    FastLED.delay(200);
    _player->play(SND_SOUNDS_0023_ANNOUNCER_VICTORY);
}
void GameAudioManager::overtime(){
  if (_overtimeTimer->canRun() ){
    _player->play(SND_SOUNDS_0017_ANNOUNCER_OVERTIME2);
    #ifdef BP_DEBUG
    Serial.print(F("Overtime"));
    #endif
  }
}
void GameAudioManager::starts_in_seconds(int secs){

  if ( _startTimeTimer->canRun() ){
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


void GameAudioManager::ends_in_seconds(int secs){
    #ifdef BP_DEBUG
    Serial.print(F("Ends in "));
    Serial.print(secs);
    Serial.print(F(" seconds!"));
    #endif  
  if ( _endTimeTimer->canRun() ){
    
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