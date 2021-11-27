#ifndef __INC_SERVO_CLOCK_DISPLAY_H
#define __INC_SERVO_CLOCK_DISPLAY_H
#include <Arduino.h>
#include <ClockDisplay.h>

#define SECONDS_PER_MINUTE 60
#define BASE_POS 0
#define DELTA 45

enum COLOR_POS {
  SERVO_POS_BLACK = BASE_POS,
  SERVO_POS_YELLOW = BASE_POS + DELTA,
  SERVO_POS_BLUE = BASE_POS - DELTA,
  SERVO_POS_RED = BASE_POS + 2* DELTA,
  SERVO_POS_CENTER=BASE_POS
};

void servo_clock_init();
void servo_clock_update_number(int value, ClockColor color);
void servo_clock_update_time(int value_seconds, ClockColor color);
void servo_clock_update_all_digits_to_map_symbol(char c , ClockColor color); //must be from the seven segment servo map
void servo_clock_blank();
void servo_clock_null(ClockColor color);

#endif