#include "Arduino.h"
#ifndef OptiWheel_h
#define OptiWheel_h
class OptiWheelFeedback{

	public:
		OptiWheelFeedback();
		void pulse();
    boolean update();
		double getRPM();
    long getPulseRate();
	private:
    volatile unsigned long prevIsrUpdateMicros;
    volatile unsigned long prevUpdateMicros;
    volatile unsigned long currentPulseRate;
		volatile unsigned int pulseCount;
    volatile boolean hasNewData;
};
#endif
