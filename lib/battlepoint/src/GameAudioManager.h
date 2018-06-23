#ifndef __INC_AUDIO_H
#define __INC_AUDIO_H
#include <Arduino.h>
#include <Teams.h>
#include <CooldownTimer.h>
#include <DFRobotDFPlayerMini.h>
class GameAudioManager {
  public:
    GameAudioManager(DFRobotDFPlayerMini* _player, long cpAlertIntervalMilliSeconds);
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
#endif

