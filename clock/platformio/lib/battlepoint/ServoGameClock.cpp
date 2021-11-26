#include <Arduino.h>
#include <ServoGameClock.h>
#include <SevenSegmentMap.h>
#include <ArduinoLog.h>
//from https://github.com/bremme/arduino-tm1637/blob/master/src/SevenSegmentTM1637.cpp

#define SERVO_POS_BASE 0
#define SERVO_POS_OFFSET 45

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
            return SERVO_POS_BASE + SERVO_POS_OFFSET;
        }
        else if ( color == ClockColor::RED){
            return SERVO_POS_BASE - SERVO_POS_OFFSET;
        }
        else if ( color == ClockColor::BLUE){
            return SERVO_POS_BASE + 2*SERVO_POS_OFFSET;
        }
    }
    return SERVO_POS_BASE;
}
void setServoAngle(int digitNum, int positionNum, int angle){

    //TODO:populate, send servo command directly to servos from here
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
void setServoToChar(int digitNum, uint8_t value , ClockColor color){
    Log.noticeln("DIGIT %d--> %B %c",value,charForColor(color));
    //for each of the segments
    for( int i=0;i<8;i++){
        int angle=getServoAngleFromColor(bitRead(value,i),color);
        setServoAngle(digitNum,i, angle );
    }   
}

void updateNumber(int value, ClockColor color){
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
    setServoToChar(ServoClockDigit::LEFT, leftSegmentValues, color);
    setServoToChar(ServoClockDigit::RIGHT, rightSegmentValues, color);
}

void updateTime(int value_seconds, ClockColor color){
    int value_to_show = value_seconds;
    if ( value_seconds > SECONDS_PER_MINUTE){
        value_to_show = value_seconds / SECONDS_PER_MINUTE;
    }
    updateNumber(value_to_show,color);
}
void setAllDigitsToValue(char c, ClockColor color){
    setServoToChar(ServoClockDigit::LEFT, c, color);
    setServoToChar(ServoClockDigit::RIGHT, c, color);
}
void blank(){
    setAllDigitsToValue(SEG_CHAR_SPACE,ClockColor::BLACK);
}
void null(ClockColor color){
    setAllDigitsToValue(SEG_CHAR_MIN,color);
}
