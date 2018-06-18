#include "LedMeter.h"
LedMeter:: LedMeter(CRGB* new_leds, LedRange* new_ranges, uint8_t new_ranges_cnt ){
  leds = new_leds;
  ranges_cnt = new_ranges_cnt;
  ranges = new_ranges;
  value = 0;
  fgColor = CRGB::Black;
  bgColor = CRGB::Black;
};

void LedMeter::init(){

};

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
  //Serial.print("Updating");
  //Serial.print(ranges_cnt);
  struct LedRange* ptr = ranges;
  for ( int i=0;i<ranges_cnt;i++,ptr++){
    //Serial.print("UpdateRange");
    updateRange(ptr);
  }
}

void LedMeter::updateRange(LedRange* range){
  //Serial.print("StartIndex=");
  //Serial.println(range->startIndex);
  //Serial.print("EndIndex=");
  //Serial.println(range->endIndex);
  //Serial.print("MaxValue=");
  //Serial.println(maxValue);

  int startIndex = range->startIndex;
  int endIndex = range->endIndex;
  int pixels_on_index = 0;

  //Serial.print(pixels_on_index);
  //Serial.print(",v=");
  //Serial.print(value);
  //Serial.println("");
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
