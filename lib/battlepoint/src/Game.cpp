#include <Game.h>
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

void Game::updateAllMetersToColors(TeamColor fg, TeamColor bg){
    _timer1Meter->setColors(fg,bg);
    _timer2Meter->setColors(fg,bg);
    _ownerMeter->setColors(fg,bg);
    _captureMeter->setColors(fg,bg);  
};

void Game::endGameDisplay( void (*delay_function)() ){
    long start_time = millis();
    long end_flash_time = start_time + (long)END_GAME_FLASH_SECONDS*1000;
    TeamColor winnerColor = getTeamColor(_winner);
    while( millis() < end_flash_time ){
        updateAllMetersToColors(TeamColor::BLACK, TeamColor::BLACK);
        delay_function();
        updateAllMetersToColors(winnerColor, TeamColor::BLACK);
        delay_function();
    }; 
};

void Game::endGameWithWinner(Team winner){    
    TeamColor winnerColor = getTeamColor(winner);
    _winner = winner;  
    _audio->victory(winner);

    updateAllMetersToColors(winnerColor, TeamColor::BLACK);

    _timer1Meter->setToMax();
    _timer2Meter->setToMax();
    _ownerMeter->setToMax();
    _captureMeter->setToMax();

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