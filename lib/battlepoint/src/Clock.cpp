#include <Clock.h>
#include <Arduino.h>


long RealClock::milliseconds(){
    return millis();
};

long TestClock::milliseconds() {
    return _currentTime;
};
void TestClock::setTime(long currentTime){
    _currentTime = currentTime;
};
void TestClock::addTime(long milliSeconds){
    _currentTime += milliSeconds;
}
