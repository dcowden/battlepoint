#ifndef __INC_GAME_H
#define __INC_GAME_H
#include <Arduino.h>
#include <GameAudioManager.h>
#include <ControlPoint.h>
#include <LedMeter.h>
#include <Teams.h>
#include <Proximity.h>

typedef struct {
     uint8_t bp_version;
     uint8_t volume;
     uint8_t brightness;
     
     GameOptions options;
} EEPromSettingsData;

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
    long timeRemainingSeconds;
    GameOptions gameOptions;
    Team winner;
    boolean isover;
    boolean isrunning;
    int redSecondsAccumulated;
    int bluSecondsAccumulated;
} GameStatistics;

class Game {
  public:
    Game(  ControlPoint* controlPoint,
           GameOptions gameOptions,
           GameAudioManager* audioManager,
           Proximity* proximity,
           LedMeter* ownerMeter,
           LedMeter* captureMeter,
           LedMeter* timer1,
           LedMeter* timer2 );
    void start();
    void update();
    void end();
    GameStatistics getStatus();
    Team getWinner();
    void endGameDisplay( void (*delay_function)() );
    int getAccumulatedSeconds(Team t);
    int getRemainingSecondsForTeam(Team t);
    int getRemainingSeconds();
    int getSecondsElapsed();
    boolean isRunning();
    GameOptions getOptions();
    virtual Team checkVictory(){ return Team::NOBODY; } ;
    virtual boolean checkOvertime() { return false; };
    virtual int getRemainingSeconds(){ return 0;};
    int getRemainingSeconds(Team t);
    virtual void init();

  protected:
    GameOptions _options;
    GameAudioManager* _audio;
    ControlPoint* _controlPoint;
    LedMeter* _ownerMeter;
    LedMeter* _captureMeter;
    LedMeter* _timer1Meter;
    LedMeter* _timer2Meter;
    Proximity* _proximity;    
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
        virtual int getRemainingSeconds();
        virtual void init();   

    private:
        boolean checkVictory(Team t);  
};

class ADGame : public Game{
    public:
        virtual Team checkVictory();
        virtual boolean checkOvertime();
        virtual int getRemainingSeconds();
        virtual void init();  
};
class CPGame : public Game{
    public:
        virtual Team checkVictory();
        virtual boolean checkOvertime();
        virtual int getRemainingSeconds();
        virtual void init();   
};

#endif