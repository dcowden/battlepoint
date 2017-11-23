#include "battlepoint.h"
#include <LiquidCrystal.h>
#define END_GAME_FLASH_INTERVAL 80
#define END_GAME_FLASH_SECONDS 15
/**
  In koth mode, each team must hold the point
  for the timelimit to win. each team has a different timer.

  In AD mode, the attacking team is always red, and must
  capture before time runs out

  In CP mode, there is no time limit, the point must simply be
  captured. the meters count forward time for each team

  Important: this class is designed so that it can run
  MORE FREQUENTLY than once per control point update interval.
  It is important that times are added based on real elapsed times



**/
Game::Game(  ControlPoint* controlPoint,
           GameOptions* gameOptions,
           EventManager* eventManager,
           Proximity* proximity,
            LedMeter* ownerMeter,
            LedMeter* captureMeter,           
            LedMeter* timer1,
             LedMeter* timer2 ){
    _gameOptions = gameOptions;
    _eventManager = eventManager;
    _controlPoint = controlPoint;
    _proximity = proximity;
    _timer1 = timer1;
    _timer2 = timer2;
    _ownerMeter = ownerMeter;
    _captureMeter = captureMeter;
    _timer2->reverse();
};
//TRODO: should be same as endGame
void Game::end(){
  _winner = NOBODY;
  _eventManager->cancelled();
}
void Game::start(){
   _timer1->init();
   _timer2->init();
   _eventManager->init(_gameOptions->getCaptureButtonThresholdSeconds());
   _controlPoint->init(_gameOptions->getCaptureSeconds());
   
   _startTime = millis();
   if ( _gameOptions->getMode() == GAME_MODE_KOTH ){
      _timer1->fgColor(CRGB::Red);
      _timer1->bgColor(CRGB::Black);
      _timer2->fgColor(CRGB::Blue);
      _timer2->bgColor(CRGB::Black);
   }
   else if ( _gameOptions->getMode() == GAME_MODE_AD ){
     _controlPoint->setRedCapture(false);
     _timer1->fgColor(CRGB::Yellow);
     _timer1->bgColor(CRGB::Black);
     _timer2->fgColor(CRGB::Yellow);
     _timer2->bgColor(CRGB::Black);
   }
   else{ //game mode CP
     _timer1->fgColor(CRGB::Red);
     _timer1->bgColor(CRGB::Black);
     _timer2->fgColor(CRGB::Blue);
     _timer2->bgColor(CRGB::Black);
   }

    _winner = NOBODY;
    _redMillis=0 ;
    _bluMillis=0 ;
   _lastUpdateTime = millis();
   _startTime = 0;
   _timer1->setMaxValue(_gameOptions->getTimeLimitSeconds());
   _timer2->setMaxValue(_gameOptions->getTimeLimitSeconds());
   _eventManager->game_started();
   _shouldAnnounceOvertime = true;

};

void Game::endGame(uint8_t winner){
    Serial.println(F("Ending Game"));
    CRGB winnerColor = get_team_color(winner);
    _winner = winner;  
    _timer1->fgColor(winnerColor);
    _timer2->fgColor(winnerColor);
    _timer1->setToMax();
    _timer2->setToMax();
    _eventManager->victory(winner);
    _controlPoint->endGame(winner);

    long start_time = millis();
    long end_flash_time = start_time + (long)END_GAME_FLASH_SECONDS*1000;
    while( millis() < end_flash_time ){
        _timer1->fgColor(CRGB::Black);
        _timer2->fgColor(CRGB::Black);
        _ownerMeter->fgColor(CRGB::Black);
        _captureMeter->fgColor(CRGB::Black);
        FastLED.show();
        FastLED.delay(END_GAME_FLASH_INTERVAL);
        _timer1->fgColor(winnerColor);
        _timer2->fgColor(winnerColor);
        _ownerMeter->fgColor(winnerColor);
        _captureMeter->fgColor(winnerColor);
        FastLED.show();
        FastLED.delay(END_GAME_FLASH_INTERVAL);                
    };
    
    
}

boolean Game::isOver(){
  return _winner != NOBODY;
}
int Game::getRedTimeLeftSeconds(){
  return (_gameOptions->getTimeLimitMilliSeconds() - _redMillis)/1000;
};
int Game::getBluTimeLeftSeconds(){
  return (_gameOptions->getTimeLimitMilliSeconds() - _bluMillis)/1000;
};
uint8_t Game::winner(){
  return _winner;
};
boolean Game::isRunning(){
  return ! isOver();
};
long Game::getRedMillis(){
  return _redMillis;
}
long Game::getBluMillis(){
  return _bluMillis;
}
void Game::update(){
     
  if ( isOver() ){
    return;
  }
  //Serial.println("X");  
  //update capture status on the control point
  _controlPoint->update(_proximity );

  long millisSinceLastUpdate = millis() - _lastUpdateTime;
  int redSecondsLeft = getRedTimeLeftSeconds();
  int bluSecondsLeft = getBluTimeLeftSeconds();

  
  if ( _gameOptions->getMode() == GAME_MODE_KOTH ){
 
      if ( _controlPoint->getOwner() == RED ){
        _redMillis += millisSinceLastUpdate;
      }
      else if ( _controlPoint->getOwner() == BLU ){
        _bluMillis += millisSinceLastUpdate;
      }
      
      //check for game over and overtime
      if ( redSecondsLeft <= 0 ){
        if ( _controlPoint->capturedBy(RED)){
          endGame(RED);  
          return;     
        }
        else{
          if ( _shouldAnnounceOvertime ){
            _eventManager->overtime();
            _shouldAnnounceOvertime = false;
          }
        }
      }
      if (bluSecondsLeft <= 0 ){
        if ( _controlPoint->capturedBy(BLU)){
          endGame(BLU);
          return;
        } 
        else{
          if ( _shouldAnnounceOvertime ){
            _eventManager->overtime();
            _shouldAnnounceOvertime = false;
          }
        }        
      }
      
      if ( _controlPoint->getOwner() == BLU ){
         _eventManager->ends_in_seconds(bluSecondsLeft);  
      }
      else if ( _controlPoint->getOwner() == RED ){
        _eventManager->ends_in_seconds(redSecondsLeft);  
      }      
  }
  else if ( _gameOptions->getMode() == GAME_MODE_AD ){

    _eventManager->ends_in_seconds(redSecondsLeft);
    
    _redMillis += millisSinceLastUpdate;
    _bluMillis += millisSinceLastUpdate;
    if ( _controlPoint->capturedBy(BLU) ){
      endGame(BLU);
      return;
    }

    if ( redSecondsLeft <= 0 ){
      if ( _controlPoint->getCapturing()  == BLU ){
          if ( _shouldAnnounceOvertime ){
            _eventManager->overtime();
            _shouldAnnounceOvertime = false;
          }              
      }
      else{
          endGame(RED); 
          return;       
      }
    }
  }
  else{  //GAME_MODE_CP
    if ( _controlPoint->getOwner() == RED ){
      _redMillis += millisSinceLastUpdate;
    }
    else if ( _controlPoint->getOwner() == BLU ){
      _bluMillis += millisSinceLastUpdate;
    }

    //this kind of game ends when time is up. the winner
    //is the one with the most time
    long currentTime= millis();
    
    if ( (currentTime - _startTime)/1000 > _gameOptions->getTimeLimitSeconds() ){
       if ( _redMillis > _bluMillis ){
          endGame(RED);  
          return;      
       }
       else{
          endGame(BLU);
          return;
       }
    }
  }

  _timer1->setValue(redSecondsLeft);
  _timer2->setValue(bluSecondsLeft);
  _lastUpdateTime = millis();  


};
