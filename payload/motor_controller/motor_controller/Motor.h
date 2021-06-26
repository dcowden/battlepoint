#include "Arduino.h"
#include "DualVNH5019MotorShield.h"
#ifndef Motor_h
#define Motor_h
class Motor{
	public:
		Motor (DualVNH5019MotorShield* _theShield, boolean _isLeft);
		void setVelocity( double velocity );
    double getVelocity();
	private:
    DualVNH5019MotorShield* motorShield;
    double velocity;
		boolean isLeft;
    void update();
};
#endif
