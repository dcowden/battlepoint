#include <Arduino.h>
#include <FastLED.h>
#include <Teams.h>

#ifndef __INC_LEDMETER_H
#define __INC_LEDMETER_H

struct LedRange {
    //zero based
    uint8_t startIndex;
    uint8_t endIndex;
};

CRGB getFastLEDColor(TeamColor tc);

class Meter {
  public:
    Meter ();
    void setValue(int value );
    void setMaxValue(int value);
    void setToMax();
    void setToMin();
    int getValue();
    int getMaxValue();
    virtual void update() = 0;
  private:
    int value;
    int maxValue;
};

class LedMeter : public Meter{

  //a meter which has two colors, and can represent a value between two boundaries
  public:
    LedMeter (CRGB* leds, LedRange* ranges, uint8_t new_ranges_cnt,CRGB new_fgcolor, CRGB new_bgcolor );
    void setColors(TeamColor fg, TeamColor bg);
    virtual void update();

  private:
    CRGB getFastLEDColor(TeamColor tc);
    void setFgColor(CRGB color);    
    void setBgColor(CRGB color);  
    CRGB* leds;
    LedRange* ranges;
    uint8_t ranges_cnt;    
    CRGB fgColor;
    CRGB bgColor;   
    void updateRange(LedRange* range);
};

class SimpleMeter: public Meter{
  public:
    virtual void update();
};


#endif