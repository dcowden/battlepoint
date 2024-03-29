#include <Arduino.h>
#include <ServoGameClock.h>
#include <SevenSegmentMap.h>
#include <ArduinoLog.h>
#include "PCA9685.h"
#include <Wire.h>
#include <FastLED.h>

//from https://github.com/bremme/arduino-tm1637/blob/master/src/SevenSegmentTM1637.cpp
#define SECONDS_PER_MINUTE 60
#define BASE_POS 0
#define DELTA 43
#define DIAL_SERVO_CHANNEL 15

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


/*
CRGB getLedColorForClockColor(ClockColor color){
    if ( color == ClockColor::YELLOW){
        return CRGB::Yellow;
    }
    else if ( color == ClockColor::RED){
        return CRGB::Red;
    }
    else if ( color == ClockColor::BLUE){
        return CRGB::Blue;
    }
}*/

float getServoAngleFromSeconds(int seconds){
    //zero, 20 is the top
    //servo 180 degrees = 1 rotation
    const int ANGLE_FOR_20 = 63;
    const int ANGLE_FOR_0 = -89;
    int INCLUDED_ANGLE = ANGLE_FOR_20 - ANGLE_FOR_0;
    
    if ( seconds > 1200 || seconds < 0 ){
        Serial.println("Error: Seconds out of bound for getServoAngleFromSeconds");
        return 0;
    }    
    float DEGREES_PER_SECOND = (float)INCLUDED_ANGLE / 1200.0;
    float angle = (float)seconds* DEGREES_PER_SECOND;
    
    float r = (float)ANGLE_FOR_0 + angle;
    Serial.println("");Serial.print("****Angle****:");
    Serial.println(r);
    return r; //center on the bottom of the clock at zero  

}
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

char getDebugSegmentChar(int bit_num, char v){
    int t = bitRead(v,bit_num);
    if ( t == 1){
        return '*';
    }
    else{
        return ' ';
    }
}
void printDigitRow(char c1, char c2, char c3, char c4, char c5, char c6, char c7, char c8){
    Serial.print("|");Serial.print(c1);Serial.print(c2);Serial.print(c3);Serial.print(c4);Serial.print("|");Serial.print(c5);Serial.print(c6);Serial.print(c7);Serial.print(c8);Serial.println("|");
}

void debugPrintdigits( char v1, char v0,ClockColor cc ){
    const char c00 = getDebugSegmentChar(0,v0);
    const char c10 = getDebugSegmentChar(1,v0);
    const char c20 = getDebugSegmentChar(2,v0);
    const char c30 = getDebugSegmentChar(3,v0);
    const char c40 = getDebugSegmentChar(4,v0);
    const char c50 = getDebugSegmentChar(5,v0);
    const char c60 = getDebugSegmentChar(6,v0);
    const char c01 = getDebugSegmentChar(0,v1);
    const char c11 = getDebugSegmentChar(1,v1);
    const char c21 = getDebugSegmentChar(2,v1);
    const char c31 = getDebugSegmentChar(3,v1);
    const char c41 = getDebugSegmentChar(4,v1);
    const char c51 = getDebugSegmentChar(5,v1);
    const char c61 = getDebugSegmentChar(6,v1);    
    const char SP = ' ';
    Serial.print("+-");Serial.print(charForColor(cc));Serial.print(charForColor(cc));Serial.println("-|----+");
    printDigitRow(SP,c01,c01,SP,SP,c00,c00,SP);
    printDigitRow(c51,SP,SP,c11,c50,SP,SP,c10);
    printDigitRow(c51,SP,SP,c11,c50,SP,SP,c10);
    printDigitRow(SP,c61,c61,SP,SP,c60,c60,SP);
    printDigitRow(c41,SP,SP,c21,c40,SP,SP,c20);
    printDigitRow(c41,SP,SP,c21,c40,SP,SP,c20);
    printDigitRow(SP,c31,c31,SP,SP,c30,c30,SP);    
    Serial.println("+----|----+");
}

void set_servo_to_char(int digitNum, uint8_t value , ClockColor color){    
    long start_ms = millis();
    //for each of the segments
    for( int i=0;i<8;i++){
        int angle=getServoAngleFromColor(bitRead(value,i),color);
        //int channel = getServoChannelForDigitAndSegment(digitNum, i);
        setServoAngle(digitNum,i,angle);
        //pwms[i] = servoHelper.pwmForAngle(angle);        
    }
    Log.noticeln("set_servo_char: %l ms",(millis()-start_ms));
    
}
void updateDialAngle(float angle){
    pwmController.setChannelPWM(DIAL_SERVO_CHANNEL, servoHelper.pwmForAngle(angle));
}
void updateDial(int dial_value){
    float dialAngle = getServoAngleFromSeconds(dial_value);
    pwmController.setChannelPWM(DIAL_SERVO_CHANNEL, servoHelper.pwmForAngle(dialAngle));
}

void servo_clock_update_number(int value, ClockColor color){
    int minutes_left = value / 60;
    updateDial(minutes_left);

    if ( value > 99 ){
        Serial.println("Cant Encode Integer Value > 99");
    }
    if ( value < 0){
        Serial.println("Cant Encode Integer Value <0");
    }    
    int leftDigit = value / 10;
    int rightDigit = value % 10;
    Log.infoln("Left: %d, Right: %d",leftDigit, rightDigit);
    uint8_t leftSegmentValues = encode(leftDigit);
    uint8_t rightSegmentValues = encode(rightDigit);
    debugPrintdigits(leftSegmentValues,rightSegmentValues,color);
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
