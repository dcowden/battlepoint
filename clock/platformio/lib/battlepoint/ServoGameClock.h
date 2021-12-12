#ifndef __INC_SERVO_CLOCK_DISPLAY_H
#define __INC_SERVO_CLOCK_DISPLAY_H
#include <Arduino.h>
#include <GameClock.h>
#include <FastLED.h>
void servo_clock_init();
void servo_clock_update_number(int value, ClockColor color);
void servo_clock_update_time(int value_seconds, ClockColor color);
void servo_clock_update_all_digits_to_map_symbol(char c , ClockColor color); //must be from the seven segment servo map
void servo_clock_blank();
void servo_clock_null(ClockColor color);
CRGB getLedColorForClockColor(ClockColor color);
void updateDial(int dial_value);
void updateDialAngle(float angle);
#endif