#include "LedMeter.h"
#include "Teams.h"
#define DEFAULT_MAX 100

CRGB LedMeter::getFastLEDColor(TeamColor tc){
  switch(tc){
    case TeamColor::RED:
        return CRGB::Red;
    case TeamColor::BLUE:
        return CRGB::Blue;
    case TeamColor::BLACK:
        return CRGB::Black;
    case TeamColor::YELLOW:
        return CRGB::Yellow;
    case TeamColor::AQUA:
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
  value = 0;
  fgColor = new_fgcolor;
  bgColor = new_bgcolor;
  maxValue = DEFAULT_MAX;
  init();  
};

void LedMeter::setColors(TeamColor fg, TeamColor bg){
  setFgColor(getFastLEDColor(fg));
  setBgColor(getFastLEDColor(bg));
}

void LedMeter::setValue(int new_value){
  value = new_value;
  update();
};
void LedMeter::setToMax(){
  setValue(maxValue);
};
void LedMeter::setToMin(){
  setValue(0);
};
void LedMeter::setMaxValue(int new_max_value){
  maxValue = new_max_value;
};
void LedMeter::setFgColor(CRGB new_color){
  fgColor = new_color;
};
void LedMeter::setBgColor(CRGB new_color){
  bgColor = new_color;
};
int LedMeter::getValue(){
  return value;
};
int LedMeter::getMaxValue(){
  return maxValue;
};

void LedMeter::update(){
  struct LedRange* ptr = ranges;
  for ( int i=0;i<ranges_cnt;i++,ptr++){
    updateRange(ptr);
  }
}

void LedMeter::updateRange(LedRange* range){

  int startIndex = range->startIndex;
  int endIndex = range->endIndex;
  int pixels_on_index = 0;
  if ( endIndex > startIndex){
    pixels_on_index  = map(value,0,maxValue,startIndex-1,endIndex);
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
    pixels_on_index  = map(value,0,maxValue,startIndex+1,endIndex);
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
