#ifndef __INC_CONTROLPOINT_H
#define __INC_CONTROLPOINT_H
#include <Arduino.h>
#include "Proximity.h"
#include "Teams.h"

class BaseControlPoint {
  public:
    Team getOwner();
    boolean isOwnedBy(Team t);
    boolean isOn(Team t);
    boolean isContested();
    Team getCapturing();
    void setOwner(Team owner);
    void setRedCaptureEnabled(boolean redCapture);
    void setBluCaptureEnabled(boolean bluCapture);
    boolean getRedCaptureEnabled();
    boolean getBluCaptureEnabled();    
    virtual void update() = 0;

  protected:
    int _secondsToCapture;
    Team _owner;
    Team _on;
    Team _capturing;    
    long _captureMillis;
    boolean _contested;
    long _lastUpdateTime;
    boolean _enableBluCapture;
    boolean _enableRedCapture;

};

class ControlPoint: public BaseControlPoint {
  public:
    ControlPoint(Proximity* proximity);
    void update();
    int getPercentCaptured();
    void init(int secondsToCapture);

  private:
    Proximity* _proximity;
    void _check_capture();
    void _dec_capture(long millis);
    void _inc_capture(long millis);
  };

//for testing
class TestControlPoint: public BaseControlPoint {
  public:
    void setCapturingTeam(Team t);
    void setOn(Team t);
    void update();
};
#endif