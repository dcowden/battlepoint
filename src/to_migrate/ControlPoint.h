#include "LedMeter.h"
#include "Proximity.h"
#include "Teams.h"
class ControlPoint {
  public:
    ControlPoint(
      LedMeter* ownerMeter,
      LedMeter* captureMeter,
      LedMeter* bluSwitch,
      LedMeter* redSwitch);
    void update(Proximity* proximity );
    boolean captured();
    boolean capturedBy(Team owner);
    Team getOwner();
    Team getOn();
    Team getCapturing();
    void setOwner(Team owner);
    long getValue();
    CRGB getOwnerColor();
    CRGB getCapturingColor();
    void init(int secondsToCapture);
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
    LedMeter* _captureMeter;
    LedMeter* _ownerMeter;
    LedMeter* _redSwitch;
    LedMeter* _bluSwitch;
    boolean _enableBluCapture;
    boolean _enableRedCapture;
    boolean _shouldContestMessage;
    void _print_team(uint8_t team);
    void _check_capture();
    void _dec_capture(long millis);
    void _inc_capture(long millis);
  };