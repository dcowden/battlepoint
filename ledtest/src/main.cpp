#include <Arduino.h>
#include <FastLED.h>
#include <LedMeter.h>


#define NUM_LEDS 8
#define LED_PIN 12

CRGB leds[NUM_LEDS];
LedMeter meter;

void setMeterValue(CRGB color, int val ){
  meter.val = val;
  meter.fgColor = color;
  updateLedMeter(&meter);
}

void setup() {
  setCpuFrequencyMhz(240);
  Serial.begin(115200);
  Serial.setTimeout(500);
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  initMeter(&meter,"leftTop",leds,0,NUM_LEDS); 
  setMeterValues(&meter,0,NUM_LEDS+1,CRGB::Blue,CRGB::Black);
}

void setSingleLedBrightness(int i, int brightness){
  leds[i].fadeToBlackBy(255-brightness);
}


int colorFlag =0;
CRGB currentColor = CRGB::Black;

void toggleColor(){
  switch ( colorFlag ){
    case(0):
      currentColor = CRGB::Blue;
      break;
    case(1):
      currentColor = CRGB::Salmon;
      break;
    case(2):
      currentColor = CRGB::Green;
      break;
    case(3):
      currentColor = CRGB::White;
      break;
    case(4):
      currentColor = CRGB::Aqua;
      break;      
  }
  colorFlag += 1;
  if( colorFlag > 4 ){
    colorFlag = 0;
  }

}
void loop() {  
  
  toggleColor();
  for(int i = 0;i<=NUM_LEDS;i++){
     setMeterValue(currentColor,i);     
     //setSingleLedBrightness(2,128);
     //setSingleLedBrightness(3,80);
     //setSingleLedBrightness(4,50);
     FastLED.show();
     FastLED.delay(200);
  }
  
}
