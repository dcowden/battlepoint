#include <Arduino.h>

//start can be less than end, in which case, we go backwards
struct LedRange {
    uint8_t startIndex;
    uint8_t endIndex;
}

class LedMeter {
  //a meter which has two colors, and can represent a value between two boundaries
   
  public:
    LedMeter (CRGB* leds, LedRange* ranges, unit8_t ranges_cnt);
    void setValue(int value );
    void setMaxValue(int value);
    void setToMax();
    void setToMin();
    void fgColor ( CRGB color );
    void reverse();
    void bgColor ( CRGB color );
    int getValue();
    int getMaxValue();
    void init();

  private:
    CRGB* leds;
    LedRange* ranges;
    unit8_t ranges_cnt;
    int value;
    int maxValue;
    CRGB fgColor;
    CRGB bgColor;
    void update();

};