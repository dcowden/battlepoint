#include <LedMeter.h>
#include <FastLED.h>
//#include <Teams.h>

void initMeter ( LedMeter* meter, const char* name, CRGB* leds, int startIndex, int endIndex ){
    meter->name = name;
    meter->startIndex = startIndex;
    meter->endIndex = endIndex;
    meter->leds = leds;
    meter->val = 0;
    meter->max_val = DEFAULT_MAX_VAL;
    meter->fgColor = CRGB::Blue;
    meter->bgColor = CRGB::Black;
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

bool isOn ( LedMeter* meter, long current_time_millis){

  long flash_interval_millis = meter->flash_interval_millis;

  if ( flash_interval_millis > 0){
     long elapsed = (current_time_millis - meter->last_flash_millis);
     
     if ( elapsed >= flash_interval_millis){
       int old_state = meter->flash_state;
       int new_state = ! meter->flash_state;
       meter->flash_state = new_state;

       meter->last_flash_millis = current_time_millis;
       if ( meter->flash_state == 0 ){
         
         return true;
       }
       else{
         
         return false;
       }
     }
     else{
       if ( meter->flash_state){
         return false;
       }
       else{
         return true;
       }
     }
  }
  return true;
}


//blank is different than value=0: the background color for blank is always 
//black (off)

void blankLedMeter( LedMeter* meter ){
  //TODO: updateLedMeter could take override values? 
  CRGB originalFgColor = meter->fgColor;
  CRGB originalBgColor = meter->bgColor;
  meter->fgColor = CRGB::Black;
  meter->bgColor = CRGB::Black;
  updateLedMeter(meter);
  meter->fgColor = originalFgColor;
  meter->bgColor = originalBgColor;
}

void setMeterValues (LedMeter* meter, int val, int max_val, CRGB fgColor, CRGB bgColor ){
    meter->val = val;
    meter->max_val = max_val;
    meter->fgColor = fgColor;
    meter->bgColor = bgColor;
}

void updateLedMeter(LedMeter* meter ){
  
  int indexIncrement =0;
  int total_lights = 0;
  int startIndex = meter->startIndex;
  int endIndex = meter->endIndex;

  //TODO annoying! after all this work trying NOT to go to OO, 
  //the log framwork really pushes us to implemetning printable
  //https://arduino.stackexchange.com/questions/53732/is-it-possible-to-print-a-custom-object-by-passing-it-to-serial-print

  if ( startIndex < endIndex ){
    total_lights = endIndex - startIndex + 1;
    indexIncrement = 1;    
  }
  else{
    //reversed
    total_lights = startIndex - endIndex + 1;
    indexIncrement = -1;
  }

  int num_lights_on = proportionalValue( meter->val, meter->max_val, total_lights); 
  int currentIndex = startIndex;

  for(int i=0;i<total_lights;i++){
    if ( i < num_lights_on ){
      meter->leds[currentIndex] = meter->fgColor;
    }
    else{
      meter->leds[currentIndex] = meter->bgColor;
    }
    currentIndex += indexIncrement;
  }

} 
void updateLedMeter(LedMeter* meter, long current_time_millis){
   
   if ( isOn( meter, current_time_millis)){
    updateLedMeter(meter);
   }
   else{
    blankLedMeter( meter);
   }
}