#include <Arduino.h>
#include <ServoGameClock.h>
#include <SevenSegmentMap.h>
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

void setServoToDigit(int digitNum, uint8_t value , ClockColor color){
    //for each of the segments
    for( int i=0;i<8;i++){
        int angle=getServoAngleFromColor(bitRead(value,i),color);
        setServoAngle(digitNum,i, angle );
    }   
}

void updateServoValue(int value, ClockColor color){
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
    setServoToDigit(ServoClockDigit::LEFT, leftSegmentValues, color);
    setServoToDigit(ServoClockDigit::RIGHT, rightSegmentValues, color);
}

void updateServoTime(int value_seconds, ClockColor color){
    int value_to_show = value_seconds;
    if ( value_seconds > SECONDS_PER_MINUTE){
        value_to_show = value_seconds / SECONDS_PER_MINUTE;
    }
    updateServoValue(value_to_show,color);
}



