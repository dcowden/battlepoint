#ifndef __INC_UTIL_H
#define __INC_UTIL_H
#include <Arduino.h>

int millis_to_seconds(long timeInMillis){
    return (int)timeInMillis / 1000;
}

int secondsSince(long startTimeMillis){
    long now = millis();
    return (int)(now - startTimeMillis) / 1000;
}

#endif