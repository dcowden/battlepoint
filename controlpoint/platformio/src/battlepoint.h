#ifndef BP_CP_H
#define BP_CP_H
#include <FastLED.h>
#include "Arduino.h"
#include "util.h"
#include "DFRobotDFPlayerMini.h"

/////////////////////////////
//Compile in debug here
//////////////////////////////
#define BP_DEBUG 1
//////////////////////////////

struct SettingsData{
     //overall options
     uint8_t bp_version;
     uint8_t volume;
     uint8_t brightness;
     
     //game options
     int      timeLimitSeconds;
     uint8_t  mode;
     uint8_t  captureSeconds;
     uint8_t  captureButtonThresholdSeconds;
     uint8_t  startDelaySeconds;  
};

class DeadbandTimer{
  public:
    DeadbandTimer();
    boolean isInDeadband(long interval);
    boolean notInDeadband(long interval);
  private:
     long lastEvent;
};

class LedMeter {
  //a meter which has two colors, and can represent a value between two boundaries
  public:
    LedMeter (CRGB* leds, uint8_t startIndex, uint8_t ledCount );
    void setValue(int value );
    void setMaxValue(int value);
    void setToMax();
    void setToMin();
    void fgColor ( CRGB color );
    void reverse();
    void bgColor ( CRGB color );
    int getValue();
    int getMaxValue();
    void init();
  private:
    CRGB* _leds;
    uint8_t _ledCount;
    boolean _reversed;
    int _value;
    int _maxValue;
    uint8_t _startIndex;
    uint8_t _endIndex;
    CRGB _fgColor;
    CRGB _bgColor;
    void _update();

};

class GameOptions {
  public:
     GameOptions();
     void init(uint8_t mode, uint8_t captureSeconds, uint8_t buttonThresholdSeconds, uint8_t timeLimitSeconds, uint8_t startDelaySeconds);
     void set_defaults();
     int getTimeLimitSeconds();
     long getTimeLimitMilliSeconds();
     uint8_t getMode();
     uint8_t getCaptureSeconds();
     uint8_t getCaptureButtonThresholdSeconds();
     uint8_t getStartDelaySeconds();
     void setMode(uint8_t mode);
     void setCaptureSeconds(uint8_t captureSeconds);
     void setTimeLimitSeconds(int timeLimitSeconds);
     void setCaptureButtonThresholdSeconds(uint8_t captureButtonThreshold);
     void setStartDelaySeconds(uint8_t startDelaySeconds);
  private:
     int _timeLimitSeconds;
     uint8_t _mode;
     uint8_t _captureSeconds;
     uint8_t _captureButtonThresholdSeconds;
     uint8_t _startDelaySeconds;
};

class EventManager {
  public:
    EventManager(DFRobotDFPlayerMini* _player,
          DeadbandTimer* captureTimer,
          DeadbandTimer* contestTimer,
          DeadbandTimer* overtimeTimer,
          DeadbandTimer* startTimeTimer,
          DeadbandTimer* endTimeTimer
    );
    void init(int cpAlertIntervalSeconds);
    void control_point_being_captured(uint8_t team);
    void control_point_captured(uint8_t team);
    void control_point_contested();
    void starting_game();
    void game_started();
    void victory(uint8_t team);
    void overtime();
    void ends_in_seconds(int secs);
    void starts_in_seconds(int secs);
    void cancelled();
  private:
    DFRobotDFPlayerMini* _player;
    long _cpAlertIntervalMilliSeconds;
    DeadbandTimer* _captureTimer;
    DeadbandTimer* _contestTimer;
    DeadbandTimer* _overtimeTimer;
    DeadbandTimer* _startTimeTimer;
    DeadbandTimer* _endTimeTimer;
};

class Proximity {
  public:
     Proximity(GameOptions* gameOptions, uint8_t redButtonPin, uint8_t bluButtonPin);
     boolean isRedClose();
     boolean isBluClose();
     void redButtonPress();
     void bluButtonPress();
     void debugStatus();
  private:
     boolean _is_red_close;
     boolean _is_blu_close;
     uint8_t _redButtonPin;
     uint8_t _bluButtonPin;
     long last_btn_red_change_millis;
     long last_btn_blu_change_millis;
     int _timeThresholdSeconds;
     GameOptions* _gameOptions;
};

class ControlPoint {
  public:
    ControlPoint(
      LedMeter* ownerMeter,
      LedMeter* captureMeter,
      EventManager* eventManager,
      LedMeter* bluSwitch,
      LedMeter* redSwitch);
    void update(Proximity* proximity );
    void update(boolean red_close, boolean blu_close );
    boolean captured();
    boolean capturedBy(uint8_t owner);
    uint8_t getOwner();
    uint8_t getOn();
    uint8_t getCapturing();
    void endGame(uint8_t winner);
    void setOwner(uint8_t owner);
    long getValue();
    CRGB getOwnerColor();
    CRGB getCapturingColor();
    void init(
          int secondsToCapture);
    void _debug_state();
    void update_display();
    void setRedCapture(boolean redCapture);
    void setBluCapture(boolean bluCapture);
  private:
    int _secondsToCapture;
    uint8_t _owner;
    uint8_t _on;
    CRGB getTeamColor(uint8_t team);
    long _value;
    uint8_t _capturing;
    boolean _contested;

    long _lastUpdateTime;
    GameOptions* _gameOptions;
    LedMeter* _captureMeter;
    LedMeter* _ownerMeter;
    LedMeter* _redSwitch;
    LedMeter* _bluSwitch;
    EventManager* _eventManager;
    boolean _enableBluCapture;
    boolean _enableRedCapture;
    boolean _shouldContestMessage;
    void _print_team(uint8_t team);
    void _check_capture();
    void _dec_capture(long millis);
    void _inc_capture(long millis);
  };

/**
 *  The game class.
 *  TODO: since we're getting all the meters, shouldnt we initi control point ourself?
 */
class Game {
  public:
    Game(  ControlPoint* controlPoint,
           GameOptions* gameOptions,
           EventManager* eventManager,
           Proximity* proximity,
            LedMeter* ownerMeter,
            LedMeter* captureMeter,
            LedMeter* timer1,
             LedMeter* timer2 );
    void start();
    void update();
    void end();
    int getRedTimeLeftSeconds();
    int getBluTimeLeftSeconds();
    long getRedMillis();
    long getBluMillis();
    uint8_t winner();
    boolean isOver();
    boolean isRunning();
    
  private:
    uint8_t _winner;
    long _startTime;
    long _lastUpdateTime;
    long _redMillis;
    long _bluMillis;
    boolean _shouldAnnounceOvertime;
    GameOptions* _gameOptions;
    EventManager* _eventManager;
    ControlPoint* _controlPoint;
    LedMeter* _ownerMeter;
    LedMeter* _captureMeter;
    LedMeter* _timer1;
    LedMeter* _timer2;
    Proximity* _proximity;
    void updateLcd();
    void endGame(uint8_t winner);

};

#endif
