#include <LedMeter.h>
#include <FastLED.h>
#include <Teams.h>

void initMeter ( LedMeter* meter, int startIndex, int endIndex ){
    meter->startIndex = startIndex;
    meter->endIndex = endIndex;
    meter->val = 0;
    meter->max_val = DEFAULT_MAX_VAL;
    meter->fgColor = CRGB::White;
    meter->fgColor = CRGB::Black;
    meter->flash_interval_millis=FlashInterval::FLASH_NONE;    
}

void configureMeter( LedMeter* meter, int max_val, int val, CRGB fg, CRGB bg){
    meter->max_val = max_val;
    meter->val = val;
    meter->fgColor = fg;
    meter->bgColor = bg;   
}

int proportionalValue(int in_val, int in_max, int out_max ){
  return in_val * out_max / in_max;
}

bool isOn ( LedFlashState* state, long flash_interval_millis, long current_time_millis){
  if ( flash_interval_millis > 0){
     long elapsed = current_time_millis - state->last_flash_millis;
     if ( elapsed > flash_interval_millis){
       state->flash_state = ! state->flash_state;
       state->last_flash_millis = current_time_millis;
       if ( state->flash_state == 0 ){
         return true;
       }
       else{
         return false;
       }
     }
  }
  return true;
}


//blank is different than value=0: the background color for blank is always 
//black (off)
void blankLedMeter( CRGB* leds, LedMeter meter ){
  //note: use non pointer here so our changes do not take affect
  meter.fgColor = CRGB::Black;
  meter.bgColor = CRGB::Black;
  updateLedMeter(leds, meter);
}

void updateLedMeter(CRGB* leds, LedMeter meter ){

  int indexIncrement =0;
  int total_lights = 0;
  if ( meter.startIndex < meter.endIndex ){
    total_lights = meter.endIndex - meter.startIndex + 1;
    indexIncrement = 1;    
  }
  else{
    //reversed
    total_lights = meter.startIndex - meter.endIndex + 1;
    indexIncrement = -1;
  }

  int num_lights_on = proportionalValue( meter.val, meter.max_val, total_lights); 
  int currentIndex = meter.startIndex;

  for(int i=0;i<total_lights;i++){
    if ( i < num_lights_on ){
      leds[currentIndex] = meter.fgColor;
    }
    else{
      leds[currentIndex] = meter.bgColor;
    }
    currentIndex += indexIncrement;
  }
} 

void updateController(CRGB* leds, LedController controller, long current_time_millis){
   if ( isOn( &controller.flashState, controller.meter.flash_interval_millis, current_time_millis)){
     updateLedMeter(leds, controller.meter);
   }
   else{
     blankLedMeter( leds, controller.meter);
   }
}