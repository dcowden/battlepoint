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
    GameMode _mode;
    uint8_t captureSeconds;
    uint8_t captureButtonThresholdSeconds;
    uint8_t startDelaySeconds;
    int timeLimitSeconds;
} GameOptions;

typedef struct {
    long timeRemainingSeconds;
    GameMode mode;
    Team winner;
    boolean isover;
    boolean isrunning;
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
    
  private:
    Team _winner;
    long _startTime;
    long _lastUpdateTime;
    long _redMillis;
    long _bluMillis;
    GameOptions _gameOptions;
    GameAudioManager* _audioManager;
    ControlPoint* _controlPoint;
    LedMeter* _ownerMeter;
    LedMeter* _captureMeter;
    LedMeter* _timer1;
    LedMeter* _timer2;
    Proximity* _proximity;    
    void endGame(Team winner);

};


#endif