#include <Arduino.h>
#include <FastLED.h>
#ifndef LEDMETER_H
#define LEDMETER_H

struct LedRange {
    //zero based
    uint8_t startIndex;
    uint8_t endIndex;
};

class LedMeter {

  //a meter which has two colors, and can represent a value between two boundaries
  public:
    LedMeter (CRGB* leds, LedRange* ranges, uint8_t new_ranges_cnt,CRGB new_fgcolor, CRGB new_bgcolor );
    void setValue(int value );
    void setMaxValue(int value);
    void setToMax();
    void setToMin();
    void setFgColor(CRGB color);
    void setBgColor(CRGB color);
    int getValue();
    int getMaxValue();

    void update();

  private:
    CRGB* leds;
    LedRange* ranges;
    uint8_t ranges_cnt;    
    int value;
    int maxValue;
    CRGB fgColor;
    CRGB bgColor;   
    void updateRange(LedRange* range);
};
#endif