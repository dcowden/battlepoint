#ifndef __INC_AUDIO_H
#define __INC_AUDIO_H
#include <Arduino.h>
#include <Teams.h>
#include <CooldownTimer.h>
#include <DFRobotDFPlayerMini.h>

class EventManager {
  public:
    virtual void control_point_being_captured(Team team);
    virtual void control_point_captured(Team team);
    virtual void control_point_contested();
    virtual void starting_game();
    virtual void game_started();
    virtual void victory(Team team);
    virtual void overtime();
    virtual void ends_in_seconds(int secs);
    virtual void starts_in_seconds(int secs);
    virtual void cancelled();  
};

class AudioEventManager : public EventManager{
  public:
    AudioEventManager(DFRobotDFPlayerMini* _player, long cpAlertIntervalMilliSeconds, Clock* clock);
    void control_point_being_captured(Team team);
    void control_point_captured(Team team);
    void control_point_contested();
    void starting_game();
    void game_started();
    void victory(Team team);
    void overtime();
    void ends_in_seconds(int secs);
    void starts_in_seconds(int secs);
    void cancelled();
  private:
    DFRobotDFPlayerMini* _player;
    CooldownTimer* _captureTimer;
    CooldownTimer* _contestTimer;
    CooldownTimer* _overtimeTimer;
    CooldownTimer* _startTimeTimer;
    CooldownTimer* _endTimeTimer;
};

class TestEventManager: public EventManager{
  public:
    void control_point_being_captured(Team team);
    void control_point_captured(Team team);
    void control_point_contested();
    void starting_game();
    void game_started();
    void victory(Team team);
    void overtime();
    void ends_in_seconds(int secs);
    void starts_in_seconds(int secs);
    void cancelled();
};
#endif

