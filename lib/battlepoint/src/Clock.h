#ifndef __INC_CLOCK_H
#define __INC_CLOCK_H
#include <Arduino.h>

class Clock{
    public:
        virtual long milliseconds() = 0;

    static int millis_to_seconds(long timeInMillis){
        return (int)timeInMillis / 1000;
    };

    static int secondsSince(long startTimeMillis){
        long now = millis();
        return (int)(now - startTimeMillis) / 1000;
    };        
};

class RealClock : public Clock {
    public:
        virtual long milliseconds();
};

class TestClock : public Clock {
    public:
        TestClock(){
            _currentTime = 0;
        };
        virtual long milliseconds();
        void setTime(long millis);
        void addTime(long millis);
    private:
        long _currentTime;
};
#endif