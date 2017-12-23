#include "battlepoint.h"

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
