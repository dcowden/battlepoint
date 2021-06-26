#include "battlepoint.h"

LedMeter:: LedMeter (CRGB* leds, uint8_t startIndex, uint8_t ledCount ){
  _leds = leds;
  _value = 0;
  _maxValue = ledCount;
  _fgColor = CRGB::Black;
  _bgColor = CRGB::Black;
  _ledCount = ledCount;
  _reversed = false;
  _startIndex = startIndex;
  _endIndex = startIndex + ledCount;
};
void LedMeter::init(){
  for(int i=_startIndex;i<_endIndex;i+=1 ){
    _leds[i] = _bgColor;
  }
};
void LedMeter::reverse(){
  _reversed = !_reversed;
}
void LedMeter::setValue(int value){
  _value = value;
  _update();
};
void LedMeter::setToMax(){
  setValue(_maxValue);
};
void LedMeter::setToMin(){
  setValue(0);
};
void LedMeter::setMaxValue(int value){
  _maxValue = value;
  _update();
};
void LedMeter::fgColor(CRGB color){
  _fgColor = color;
  _update();
};
void LedMeter::bgColor(CRGB color){
  _bgColor = color;
};
int LedMeter::getValue(){
  return _value;
};
int LedMeter::getMaxValue(){
  return _maxValue;
};
void LedMeter::_update(){
  int pixels_on = map(_value,0,_maxValue,0,_ledCount);

  if ( ! _reversed ){
    for ( int i=_startIndex;i< _endIndex;i++){
      if ( i < pixels_on + _startIndex ){
        _leds[i] =_fgColor;
      }
      else{
        _leds[i] = _bgColor;
      }
    }    
  }
  else{
    for ( int i=_startIndex;i< _endIndex;i++){
      if ( i > (_endIndex - pixels_on -1 ) ){
        _leds[i] =_fgColor;
      }
      else{
        _leds[i] = _bgColor;
      }
    }      
  }

}
