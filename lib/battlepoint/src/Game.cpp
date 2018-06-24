#include <Game.h>
//TODO: have to abstract this to allow unit testing!
#include <FastLED.h>
#include <util.h>
#include <Teams.h>

#define END_GAME_FLASH_SECONDS 10
#define END_GAME_FLASH_INTERVAL_MILLISECONDS 80

Game::Game(  ControlPoint* controlPoint,
           GameOptions gameOptions,
           GameAudioManager* audioManager,
           Proximity* proximity,
           LedMeter* ownerMeter,
           LedMeter* captureMeter,
           LedMeter* timer1,
           LedMeter* timer2 ){
    _options = gameOptions;
    _audio = audioManager;
    _controlPoint = controlPoint;
    _proximity = proximity;
    _timer1Meter = timer1;
    _timer2Meter = timer2;
    _ownerMeter = ownerMeter;
    _captureMeter = captureMeter;
};

void Game::end(){
  _winner = Team::NOBODY;
  _audio->cancelled();
};

void Game::start(){ 
   
    _startTime = millis();
    init();
    _winner = Team::NOBODY;
    _redAccumulatedTimeMillis=0 ;
    _bluAccumulatedTimeMillis=0 ;
    _lastUpdateTime = millis();
    _startTime = 0;
    _timer1Meter->setMaxValue(_options.timeLimitSeconds);
    _timer2Meter->setMaxValue(_options.timeLimitSeconds);
    _audio->game_started();

};

void Game::updateAllMetersToColor(CRGB color){
    _timer1Meter->setFgColor(color);
    _timer2Meter->setFgColor(color);
    _ownerMeter->setFgColor(color);
    _captureMeter->setFgColor(color);    
};

void Game::endGameWithWinner(Team winner){    
    CRGB winnerColor = getTeamColor(winner);
    _winner = winner;  
    _audio->victory(winner);

    updateAllMetersToColor(winnerColor);

    _timer1Meter->setToMax();
    _timer2Meter->setToMax();
    _ownerMeter->setToMax();
    _captureMeter->setToMax();

    long start_time = millis();
    long end_flash_time = start_time + (long)END_GAME_FLASH_SECONDS*1000;
    while( millis() < end_flash_time ){
        updateAllMetersToColor(CRGB::Black);
        FastLED.show();
        FastLED.delay(END_GAME_FLASH_INTERVAL_MILLISECONDS);
        updateAllMetersToColor(winnerColor);
        FastLED.show();
        FastLED.delay(END_GAME_FLASH_INTERVAL_MILLISECONDS);                
    }; 
};

GameOptions Game::getOptions(){
    return _options;
};

boolean Game::isRunning(){
  return _winner == Team::NOBODY;
};

int Game::getRemainingSeconds(Team t){
  return _options.timeLimitSeconds - getAccumulatedSeconds(t);
};

int Game::getRemainingSecondsForTeam(Team t){
  if ( t == Team::RED){
      return millis_to_seconds(_redAccumulatedTimeMillis);
  }
  else if ( t == Team::BLU){
      millis_to_seconds(_bluAccumulatedTimeMillis);
  }
  else{
      return 0;
  }
};

Team Game::getWinner(){
  return _winner;
};

int Game::getSecondsElapsed(){
   return secondsSince(_startTime);
};

void Game::updateAccumulatedTime(){
    long millisSinceLastUpdate = millis() - _lastUpdateTime;
    if ( _controlPoint->getOwner() == Team::RED ){
       _redAccumulatedTimeMillis += millisSinceLastUpdate;
    }
    if ( _controlPoint->getOwner() == Team::BLU ){
       _bluAccumulatedTimeMillis += millisSinceLastUpdate;
    }

};

void Game::update(){
     
  if ( ! isRunning() ){
    return;
  }
  _controlPoint->update(_proximity );
  updateAccumulatedTime();

  Team winner = checkVictory();
  if ( winner != Team::NOBODY){
      endGameWithWinner(winner);
      return;
  }

  if ( checkOvertime() ){
      _audio->overtime();
  }

  _audio->ends_in_seconds( getRemainingSeconds() );
  _timer1Meter->setValue(getRemainingSeconds(Team::RED));
  _timer2Meter->setValue(getRemainingSeconds(Team::BLU));
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
  
};