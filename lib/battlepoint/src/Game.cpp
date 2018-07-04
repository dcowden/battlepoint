#include <Game.h>
#include <Clock.h>
#include <Teams.h>

#define END_GAME_FLASH_SECONDS 10
#define END_GAME_FLASH_INTERVAL_MILLISECONDS 80
#define NOT_STARTED  -1
void Game::init(  BaseControlPoint* controlPoint,
           GameOptions gameOptions,
           EventManager* eventManager,
           Meter* ownerMeter,
           Meter* captureMeter,
           Meter* timer1,
           Meter* timer2,
           Clock* clock ){
    _options = gameOptions;
    _events = eventManager;
    _controlPoint = controlPoint;
    _timer1Meter = timer1;
    _timer2Meter = timer2;
    _ownerMeter = ownerMeter;
    _captureMeter = captureMeter;
    _gameClock = clock;
    resetGame();
};

void Game::end(){
  _winner = Team::NOBODY;
  _events->cancelled();
};

void Game::resetGame(){
    
    gameTypeInit();
    _winner = Team::NOBODY;
    _redAccumulatedTimeMillis=0 ;
    _bluAccumulatedTimeMillis=0 ;
    _lastUpdateTime = _gameClock->milliseconds();
    _startTime = NOT_STARTED;
    _timer1Meter->setMaxValue(_options.timeLimitSeconds);
    _timer2Meter->setMaxValue(_options.timeLimitSeconds);
};

void Game::start(){
    resetGame();
    _startTime = _gameClock->milliseconds();
    _events->game_started();
};

void Game::updateAllMetersToColors(TeamColor fg, TeamColor bg){
    _timer1Meter->setColors(fg,bg);
    _timer2Meter->setColors(fg,bg);
    _ownerMeter->setColors(fg,bg);
    _captureMeter->setColors(fg,bg);  
};

void Game::endGameDisplay( void (*delay_function)() ){
    long start_time = _gameClock->milliseconds();
    long end_flash_time = start_time + (long)END_GAME_FLASH_SECONDS*1000;

    _timer1Meter->setToMax();
    _timer2Meter->setToMax();
    _ownerMeter->setToMax();
    _captureMeter->setToMax();

    TeamColor winnerColor = getTeamColor(_winner);
    while( _gameClock->milliseconds() < end_flash_time ){
        updateAllMetersToColors(TeamColor::COLOR_BLACK, TeamColor::COLOR_BLACK);
        delay_function();
        updateAllMetersToColors(winnerColor, TeamColor::COLOR_BLACK);
        delay_function();
    }; 
};

void Game::endGameWithWinner(Team winner){    
    TeamColor winnerColor = getTeamColor(winner);
    _winner = winner;  
    _events->victory(winner);
    _startTime = NOT_STARTED;
};

GameOptions Game::getOptions(){
    return _options;
};

boolean Game::isRunning(){
  return _startTime != NOT_STARTED;
};

int Game::getRemainingSecondsForTeam(Team t){
  return _options.timeLimitSeconds - getAccumulatedSeconds(t);
};

Team Game::getWinner(){
  return _winner;
};

int Game::getSecondsElapsed(){
   if ( _startTime == NOT_STARTED){
       return 0;
   }
   else{
       return Clock::secondsSince(_startTime);
   }   
};

int Game::getAccumulatedSeconds(Team t){
    if ( t == Team::RED){
        return Clock::millis_to_seconds(_redAccumulatedTimeMillis);
    }
    else if ( t == Team::BLU){
        return Clock::millis_to_seconds(_bluAccumulatedTimeMillis);
    }
    else{
        return 0;
    }
};

void Game::updateAccumulatedTime(){
    long millisSinceLastUpdate = _gameClock->milliseconds() - _lastUpdateTime;
    if ( _controlPoint->getOwner() == Team::RED ){
       _redAccumulatedTimeMillis += millisSinceLastUpdate;
    }
    else if ( _controlPoint->getOwner() == Team::BLU ){
       _bluAccumulatedTimeMillis += millisSinceLastUpdate;
    }
    _lastUpdateTime =_gameClock->milliseconds(); 
};

void Game::update(){
     
  if ( ! isRunning() ){
    return;
  }
  _controlPoint->update( );
  updateAccumulatedTime();
  

  Team winner = checkVictory();
  if ( winner != Team::NOBODY){
      endGameWithWinner(winner);
      updateDisplay(); 
      return;
  }

  if ( checkOvertime() ){
      _events->overtime();
  }
  
  _events->ends_in_seconds( getRemainingSeconds() );

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