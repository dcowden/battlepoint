#include "LedMeter.h"

LedMeter:: LedMeter(CRGB* leds, LedRange* ranges, uint8_t ranges_cnt ){
  leds = leds;
  ranges_cnt = ranges_cnt;
  value = 0;
  fgColor = CRGB::Black;
  bgColor = CRGB::Black;
};

void LedMeter::init(){
  //for(int i=_startIndex;i<_endIndex;i+=1 ){
  //  _leds[i] = _bgColor;
  // }
};

void LedMeter::setValue(int value){
  value = value;
  update();
};
void LedMeter::setToMax(){
  setValue(maxValue);
};
void LedMeter::setToMin(){
  setValue(0);
};
void LedMeter::setMaxValue(int value){
  maxValue = value;
  update();
};
void LedMeter::setFgColor(CRGB color){
  fgColor = color;
  update();
};
void LedMeter::setBgColor(CRGB color){
  bgColor = color;
  update();
};
int LedMeter::getValue(){
  return value;
};
int LedMeter::getMaxValue(){
  return maxValue;
};
void LedMeter::updateRange(LedRange* range){
  // int pixels_on = map(value,0,maxValue,0,ledCount);

  // if ( ! _reversed ){
  //   for ( int i=_startIndex;i< _endIndex;i++){
  //     if ( i < pixels_on + _startIndex ){
  //       _leds[i] =_fgColor;
  //     }
  //     else{
  //       _leds[i] = _bgColor;
  //     }
  //   }    
  // }
  // else{
  //   for ( int i=_startIndex;i< _endIndex;i++){
  //     if ( i > (_endIndex - pixels_on -1 ) ){
  //       _leds[i] =_fgColor;
  //     }
  //     else{
  //       _leds[i] = _bgColor;
  //     }
  //   }      
  // }

}
