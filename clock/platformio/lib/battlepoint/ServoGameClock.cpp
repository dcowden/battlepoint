#include <Arduino.h>
#include <ServoGameClock.h>
#include <SevenSegmentMap.h>
#include <ArduinoLog.h>
#include "PCA9685.h"
#include <Wire.h>

//from https://github.com/bremme/arduino-tm1637/blob/master/src/SevenSegmentTM1637.cpp
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

static PCA9685 pwmController(Wire);   
static PCA9685_ServoEvaluator  servoHelper;

void servo_clock_init(){
  pwmController.resetDevices();       // Resets all PCA9685 devices on i2c lin
  pwmController.init();               // Initializes module using default totem-pole driver mode, and default disabled phase balancer
  pwmController.setPWMFrequency(50);
}

// Helpers
uint8_t encode(char c) {
  if ( c < ' ') { // 32 (ASCII)
    return 0;
  }
  return pgm_read_byte_near(AsciiMap::map + c - ' ');
};

uint8_t encode(int d) {
  // can only encode single digit
  if ( d > 9 || d < 0) {
    return 0;
  };
  return pgm_read_byte_near(AsciiMap::map + d + '0' - ' ');
};


int getServoAngleFromColor(int v, ClockColor color){
    if ( v == 1){
        if ( color == ClockColor::YELLOW){
            return SERVO_POS_YELLOW;
        }
        else if ( color == ClockColor::RED){
            return SERVO_POS_RED;
        }
        else if ( color == ClockColor::BLUE){
            return SERVO_POS_BLUE;
        }
    }
    return SERVO_POS_CENTER;
}
int getServoChannelForDigitAndSegment(int digitNum, int segmentNum){
    //digit is either zero or one
    if ( digitNum == 0){
        return segmentNum;
    }
    else{
        return segmentNum + 8;
    }
}

void setServoAngle(int digitNum, int segmentNum, int angle){
    int channel = getServoChannelForDigitAndSegment(digitNum, segmentNum);
    pwmController.setChannelPWM(channel, servoHelper.pwmForAngle(angle)); 
}

char charForColor(ClockColor color){
    if ( color == ClockColor::BLACK){
        return 'K';
    }
    else if ( color == ClockColor::YELLOW){
        return 'Y';
    }
    else if (color == ClockColor::RED){
        return 'R';
    }
    else if (color == ClockColor::GREEN){
        return 'G';
    }
    else if (color == ClockColor::WHITE){
        return 'W';
    }
    else if (color == ClockColor::BLUE){
        return 'B';
    }
    return '?';
}
void set_servo_to_char(int digitNum, uint8_t value , ClockColor color){
    Log.noticeln("DIGIT %d--> %B %c",digitNum,value,charForColor(color));
    //for each of the segments
    for( int i=0;i<8;i++){
        int angle=getServoAngleFromColor(bitRead(value,i),color);
        setServoAngle(digitNum,i, angle );
    }   
}

void servo_clock_update_number(int value, ClockColor color){
    if ( value > 99 ){
        Serial.println("Cant Encode Integer Value > 99");
    }
    if ( value < 0){
        Serial.println("Cant Encode Integer Value <0");
    }    
    int leftDigit = value / 10;
    int rightDigit = value % 10;

    uint8_t leftSegmentValues = encode(leftDigit);
    uint8_t rightSegmentValues = encode(rightDigit);
    set_servo_to_char(1, leftSegmentValues, color);
    set_servo_to_char(0, rightSegmentValues, color);
}

void servo_clock_update_time(int value_seconds, ClockColor color){
    int value_to_show = value_seconds;
    if ( value_seconds > SECONDS_PER_MINUTE){
        value_to_show = value_seconds / SECONDS_PER_MINUTE;
    }
    servo_clock_update_number(value_to_show,color);
}
void servo_clock_update_all_digits_to_map_symbol(char c, ClockColor color){
    set_servo_to_char(1, c, color);
    set_servo_to_char(0, c, color);
}
void servo_clock_blank(){
    servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_SPACE,ClockColor::BLACK);
}
void servo_clock_null(ClockColor color){
    servo_clock_update_all_digits_to_map_symbol(SEG_CHAR_MIN,color);
}
