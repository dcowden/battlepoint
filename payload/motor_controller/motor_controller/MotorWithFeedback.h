#include "Arduino.h"
#include <PID_v1.h>
#include "OptiWheelFeedback.h"
#include "Motor.h"
#ifndef MotorWithFeedback_h
#define MotorWithFeedback_h
class MotorWithFeedback{
	public:
		MotorWithFeedback ( OptiWheelFeedback* the_encoder , Motor* the_motor, double Kp, double Ki, double Kd );
		void update();
    void setTargetVelocity(double target);
    double getTargetVelocity();
    void debug(char id);
    void setEnabled(boolean en);
	private:
		OptiWheelFeedback* encoder;
		PID* pid;
		Motor* motor;
    boolean directionForward;
    boolean enabled;
    double targetSpeed;
    double currentSpeed;
    double outputCommand;
};
#endif
