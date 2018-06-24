#include <Arduino.h>
#include <FastLED.h>
#ifndef __INC_LEDMETER_H
#define __INC_LEDMETER_H

struct LedRange {
    //zero based
    uint8_t startIndex;
    uint8_t endIndex;
};

CRGB getFastLEDColor(TeamColor tc);

class LedMeter {

  //a meter which has two colors, and can represent a value between two boundaries
  public:
    LedMeter (CRGB* leds, LedRange* ranges, uint8_t new_ranges_cnt,CRGB new_fgcolor, CRGB new_bgcolor );
    void setValue(int value );
    void setMaxValue(int value);
    void setToMax();
    void setToMin();
    void setColors(TeamColor fg, TeamColor bg);
    int getValue();
    int getMaxValue();
    void update();

  private:
    CRGB getFastLEDColor(TeamColor tc);
    void setFgColor(CRGB color);    
    void setBgColor(CRGB color);  
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