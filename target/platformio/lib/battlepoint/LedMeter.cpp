#include <LedMeter.h>
#include <FastLED.h>
#include <Teams.h>

int proportionalValue(int in_val, int in_max, int out_max ){
  return in_val * out_max / in_max;
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
 
  #ifdef BP_DEBUG
    Serial.print("Val=");
    Serial.println(val);  
    Serial.print("Total Lights=");
    Serial.println(total_lights);
    Serial.print("Lights On=");
    Serial.println(num_lights_on);
    Serial.print("CurrentIndex=");
    Serial.println(currentIndex);
    Serial.print("startIndex=");
    Serial.println(meter.startIndex); 
    Serial.print("indexIncrement=");
    Serial.println(indexIncrement);        
  #endif


  for(int i=0;i<total_lights;i++){
    #ifdef BP_DEBUG
      Serial.print(i);
      Serial.print(",");
      Serial.println(currentIndex);
    #endif
    if ( i < num_lights_on ){
      leds[currentIndex] = meter.fgColor;
    }
    else{
      leds[currentIndex] = meter.bgColor;
    }
    currentIndex += indexIncrement;
  }

} 