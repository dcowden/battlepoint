#include <LedMeter.h>
#include <FastLED.h>
#include <Teams.h>
#include <ArduinoLog.h>

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
void debugLedController (LedController* controller){
  Log.traceln("Meter '%s': %d/%d [%d:%d] <%d>",controller->meter->name, controller->meter->val,controller->meter->max_val,controller->meter->startIndex,controller->meter->endIndex ,controller->flashState);
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
     Log.traceln("Flash Interval=%d, elapsed=%d, current_time=%d", flash_interval_millis,elapsed,current_time_millis);
     if ( elapsed >= flash_interval_millis){
       int old_state = state->flash_state;
       int new_state = ! state->flash_state;
       Log.infoln("Toggling Controller State, %d->%d",old_state,new_state);
       state->flash_state = ! state->flash_state;

       state->last_flash_millis = current_time_millis;
       if ( state->flash_state == 0 ){
         Log.infoln("Flash:on=true");
         return true;
       }
       else{
         Log.infoln("Flash:on=false");
         return false;
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

void updateLedMeter(LedMeter* meter ){
  
  int indexIncrement =0;
  int total_lights = 0;
  int startIndex = meter->startIndex;
  int endIndex = meter->endIndex;

  //TODO annoying! after all this work trying NOT to go to OO, 
  //the log framwork really pushes us to implemetning printable
  //https://arduino.stackexchange.com/questions/53732/is-it-possible-to-print-a-custom-object-by-passing-it-to-serial-print
  Log.infoln("UpdateMeter '%s': %d/%d [%d:%d]",meter->name, meter->val,meter->max_val,startIndex,endIndex );

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
void updateController(LedController* controller, long current_time_millis){
   //updateLedMeter(controller->meter);
   
   if ( isOn( &(controller->flashState), controller->meter->flash_interval_millis, current_time_millis)){
    Log.traceln("LED Strip is ON, updating meter");
    updateLedMeter(controller->meter);
   }
   else{
    Log.traceln("LED Strip is OFF, blanking meter");
    blankLedMeter( controller->meter);
   }
}