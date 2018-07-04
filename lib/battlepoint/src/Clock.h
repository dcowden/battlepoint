#ifndef __INC_CLOCK_H
#define __INC_CLOCK_H
#include <Arduino.h>

class Clock{
    public:
        virtual long milliseconds() = 0;
};

class RealClock : public Clock {
    public:
        virtual long milliseconds();
};

class TestClock : public Clock {
    public:
        TestClock(){
            _currentTime = 0;
        }
        virtual long milliseconds();
        void setTime(long millis);
    private:
        long _currentTime;
};
#endif