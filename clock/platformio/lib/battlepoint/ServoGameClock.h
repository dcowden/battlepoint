#ifndef __INC_SERVO_CLOCK_DISPLAY_H
#define __INC_SERVO_CLOCK_DISPLAY_H
#include <Arduino.h>
#include <ClockDisplay.h>
#define SECONDS_PER_MINUTE 60

typedef enum {
    LEFT = 1,
    RIGHT = 0,
} ServoClockDigit;

void updateServoValue(int value, ClockColor color);
void updateServoTime(int value_seconds, ClockColor color);
#endif