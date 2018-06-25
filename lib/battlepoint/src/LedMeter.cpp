#include "LedMeter.h"
#include <Teams.h>

#define DEFAULT_MAX 100

Meter::Meter(){
  value=0;
  maxValue = DEFAULT_MAX;
};

void Meter::setValue(int new_value){
  value = new_value;
  update();
};
void Meter::setToMax(){
  setValue(maxValue);
};
void Meter::setToMin(){
  setValue(0);
};
void Meter::setMaxValue(int new_max_value){
  maxValue = new_max_value;
};

int Meter::getValue(){
  return value;
};
int Meter::getMaxValue(){
  return maxValue;
};

CRGB LedMeter::getFastLEDColor(TeamColor tc){
  switch(tc){
    case TeamColor::COLOR_RED:
        return CRGB::Red;
    case TeamColor::COLOR_BLUE:
        return CRGB::Blue;
    case TeamColor::COLOR_BLACK:
        return CRGB::Black;
    case TeamColor::COLOR_YELLOW:
        return CRGB::Yellow;
    case TeamColor::COLOR_AQUA:
        return CRGB::Aqua;
    default:
        return CRGB::Black;
  }
}

LedMeter::LedMeter(CRGB* new_leds, LedRange* new_ranges, 
        uint8_t new_ranges_cnt, CRGB new_fgcolor, CRGB new_bgcolor ){
  leds = new_leds;
  ranges_cnt = new_ranges_cnt;
  ranges = new_ranges;
  fgColor = new_fgcolor;
  bgColor = new_bgcolor;
  init();  
};

void LedMeter::setColors(TeamColor fg, TeamColor bg){
  setFgColor(getFastLEDColor(fg));
  setBgColor(getFastLEDColor(bg));
};

void LedMeter::setFgColor(CRGB new_color){
  fgColor = new_color;
};
void LedMeter::setBgColor(CRGB new_color){
  bgColor = new_color;
};


void LedMeter::update(){
  struct LedRange* ptr = ranges;
  for ( int i=0;i<ranges_cnt;i++,ptr++){
    updateRange(ptr);
  }
};

void LedMeter::updateRange(LedRange* range){

  int startIndex = range->startIndex;
  int endIndex = range->endIndex;
  int pixels_on_index = 0;
  if ( endIndex > startIndex){
    pixels_on_index  = map(getValue(),0,getMaxValue(),startIndex-1,endIndex);
    for ( int i=startIndex;i<= endIndex;i++){
      if ( i <= pixels_on_index  ){
        leds[i] =fgColor;
      }
      else{
        leds[i] = bgColor;
      }
    }    
  }
  else{
    pixels_on_index  = map(getValue(),0,getMaxValue(),startIndex+1,endIndex);
    for ( int i=endIndex;i<= startIndex;i++){
      if ( i >= pixels_on_index  ){
        leds[i] =fgColor;
      }
      else{
        leds[i] = bgColor;
      }
    }    
  }

}

void SimpleMeter::update(){
  //do nothing;
}