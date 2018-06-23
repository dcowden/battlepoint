#ifndef __INC_CONTROLPOINT_H
#define __INC_CONTROLPOINT_H
#include <Arduino.h>
#include "Proximity.h"
#include "Teams.h"

class ControlPoint {
  public:
    ControlPoint();
    void update(Proximity* proximity );
    Team getOwner();
    boolean isOwnedBy(Team t);
    boolean isOn(Team t);
    boolean isContested();
    Team getCapturing();
    void setOwner(Team owner);
    int getPercentCaptured();
    void init(int secondsToCapture);
    void debug_state();
    void setRedCaptureEnabled(boolean redCapture);
    void setBluCaptureEnabled(boolean bluCapture);
    boolean getRedCaptureEnabled();
    boolean getBluCaptureEnabled();
  private:
    int _secondsToCapture;
    Team _owner;
    Team _on;
    Team _capturing;    
    long _captureMillis;
    boolean _contested;
    long _lastUpdateTime;
    boolean _enableBluCapture;
    boolean _enableRedCapture;
    void _check_capture();
    void _dec_capture(long millis);
    void _inc_capture(long millis);
  };
#endif