#include "battlepoint.h"
#include <FastLED.h>
/**
 * Test
 * Connect the strands. all 4 meters should count up. Two should be reversed
 */
 void test_meters_setup(LedMeter* captureMeter, LedMeter* ownerMeter, LedMeter* timer1Meter, LedMeter* timer2Meter){
  captureMeter->init();
  ownerMeter->init();
  timer1Meter->init();
  timer2Meter->init();
  captureMeter->fgColor(CRGB::Red);
  ownerMeter->fgColor(CRGB::Blue);
  timer1Meter->fgColor(CRGB::Yellow);
  timer2Meter->fgColor(CRGB::Green);  
  captureMeter->setMaxValue(20);
  ownerMeter->setMaxValue(20);
  timer1Meter->setMaxValue(20);
  timer2Meter->setMaxValue(20);
  captureMeter->reverse();
  ownerMeter->reverse();  
 }

void test_meters_loop(LedMeter* captureMeter, LedMeter* ownerMeter, LedMeter* timer1Meter, LedMeter* timer2Meter){
  for ( int i=0;i<=20;i++){
    captureMeter->setValue(i);
    ownerMeter->setValue(i);
    timer1Meter->setValue(i);
    timer2Meter->setValue(i);
    FastLED.show();
    delay(1000);    
  }  
}

void test_buttons_setup(Proximity* proximity){
  Serial.print(F("Press the buttons to test "));
}

void test_buttons_loop(Proximity* proximity){
 
    while(true){
        proximity->debugStatus();
        delay(1000);
    }
}

 void test_controlpoint_setup(ControlPoint* cp, LedMeter* captureMeter, LedMeter* ownerMeter, LedMeter* timer1Meter, LedMeter* timer2Meter,int captureSeconds){
    captureMeter->init();
    ownerMeter->init();
    timer1Meter->init();
    timer2Meter->init();
    captureMeter->bgColor(CRGB::Black);
    ownerMeter->bgColor(CRGB::Black);
    captureMeter->setMaxValue(captureSeconds);
    ownerMeter->setToMax();
    cp->init(captureSeconds);    
 }
 
 void test_controlpoint_loop(ControlPoint* cp, Proximity* prox){
    while(true){
        cp->update(prox);
        #ifdef BP_DEBUG        
        cp->_debug_state();
        #endif
        FastLED.show();
        delay(200);
    }
 }

