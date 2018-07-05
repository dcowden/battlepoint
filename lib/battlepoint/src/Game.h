#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Arduino.h>
#include <EventManagers.h>
#include <ControlPoint.h>
#include <LedMeter.h>
#include <Teams.h>
#include <Proximity.h>
#include <Clock.h>
typedef enum {
    AD,
    KOTH,
    CP
} GameMode;

typedef struct {
    GameMode mode;
    uint8_t captureSeconds;
    uint8_t captureButtonThresholdSeconds;
    uint8_t startDelaySeconds;
    int timeLimitSeconds;
} GameOptions;

typedef struct {
     uint8_t bp_version;
     uint8_t volume;
     uint8_t brightness;
     
     GameOptions options;
} EEPromSettingsData;


class Game {
  public:
    void init(  BaseControlPoint* controlPoint,
           GameOptions gameOptions,
           EventManager* eventManager,
           Meter* ownerMeter,
           Meter* captureMeter,
           Meter* timer1,
           Meter* timer2,
           Clock* clock );
    void start();
    void update();
    void end();
    Team getWinner();
    void endGameDisplay( void (*delay_function)() );
    int getAccumulatedSeconds(Team t);
    int getRemainingSecondsForTeam(Team t);
    int getSecondsElapsed();
    boolean isRunning();
    GameOptions getOptions();
    virtual void updateDisplay(){};
    int getRemainingSeconds();
    virtual int getGameTypeRemainingSeconds(){ return 0;};
    virtual void gameTypeInit();
 
  protected:
    void resetGame();
    virtual Team checkVictory(){ return Team::NOBODY; } ;
    virtual boolean checkOvertime() { return false; };  
    
    GameOptions _options;
    EventManager* _events;
    BaseControlPoint* _controlPoint;
    Meter* _ownerMeter;
    Meter* _captureMeter;
    Meter* _timer1Meter;
    Meter* _timer2Meter; 
    Clock* _gameClock;
    long _redAccumulatedTimeMillis;
    long _bluAccumulatedTimeMillis;

  private:
    Team _winner;
    long _startTime;
    long _lastUpdateTime;
    void endGameWithWinner(Team winner);
    void updateAccumulatedTime();
    void updateAllMetersToColors(TeamColor fg, TeamColor bg);
    
};

class KothGame : public Game{
    public: 
        virtual Team checkVictory();
        virtual boolean checkOvertime();        
        virtual void gameTypeInit();
        virtual void updateDisplay();   
        virtual int getGameTypeRemainingSeconds();

    private:
        boolean checkVictory(Team t);  
};

class ADGame : public Game{
    public:
        virtual Team checkVictory();
        virtual boolean checkOvertime();        
        virtual void gameTypeInit();
        virtual void updateDisplay();
        virtual int getGameTypeRemainingSeconds();
};
class CPGame : public Game{
    public:
        virtual Team checkVictory();
        virtual boolean checkOvertime();
        virtual void gameTypeInit();
        virtual void updateDisplay();   
        virtual int getGameTypeRemainingSeconds();
};

#endif