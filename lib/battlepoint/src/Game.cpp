#include <Game.h>

Game::Game(  ControlPoint* controlPoint,
           GameOptions gameOptions,
           GameAudioManager* audioManager,
           Proximity* proximity,
           LedMeter* ownerMeter,
           LedMeter* captureMeter,
           LedMeter* timer1,
           LedMeter* timer2 ){
    _gameOptions = gameOptions;
    _audioManager = audioManager;
    _controlPoint = controlPoint;
    _proximity = proximity;
    _timer1 = timer1;
    _timer2 = timer2;
    _ownerMeter = ownerMeter;
    _captureMeter = captureMeter;

};

void Game::end(){
  _winner = Team::NOBODY;
  _audioManager->cancelled();
}
void Game::start(){
   
   _startTime = millis();
   if ( _gameOptions._mode == GameMode. ){
      _timer1->setFgColor(CRGB::Red);
      _timer1->setBgColor(CRGB::Black);
      _timer2->setFgColor(CRGB::Blue);
      _timer2->setBgColor(CRGB::Black);
   }
   else if ( _gameOptions->getMode() == GAME_MODE_AD ){
     _controlPoint->setRedCapture(false);
     _timer1->setFgColor(CRGB::Yellow);
     _timer1->setBgColor(CRGB::Black);
     _timer2->setFgColor(CRGB::Yellow);
     _timer2->setBgColor(CRGB::Black);
   }
   else{ //game mode CP
     _timer1->setFgColor(CRGB::Red);
     _timer1->setBgColor(CRGB::Black);
     _timer2->setFgColor(CRGB::Blue);
     _timer2->setBgColor(CRGB::Black);
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
    _timer1->setFgColor(winnerColor);
    _timer2->setFgColor(winnerColor);
    _timer1->setToMax();
    _timer2->setToMax();
    _eventManager->victory(winner);
    _controlPoint->endGame(winner);

    long start_time = millis();
    long end_flash_time = start_time + (long)END_GAME_FLASH_SECONDS*1000;
    while( millis() < end_flash_time ){
        _timer1->setFgColor(CRGB::Black);
        _timer2->setFgColor(CRGB::Black);
        _ownerMeter->setFgColor(CRGB::Black);
        _captureMeter->setFgColor(CRGB::Black);
        FastLED.show();
        FastLED.delay(END_GAME_FLASH_INTERVAL);
        _timer1->setFgColor(winnerColor);
        _timer2->setFgColor(winnerColor);
        _ownerMeter->setFgColor(winnerColor);
        _captureMeter->setFgColor(winnerColor);
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


void print_game_mode_text(char* buffer, GameMode mode){
  switch ( mode ){
    case GameMode::KOTH:
       strcpy(buffer, "KOTH");
       break;
    case GameMode::AD:
       strcpy(buffer,"AD");
       break;
    case GameMode::CP:
       strcpy(buffer, "CP");
       break;
    default:
      strcpy(buffer,"??");
  }
  
}