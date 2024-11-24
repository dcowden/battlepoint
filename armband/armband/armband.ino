#include <Arduino.h>
#include <FastLED.h>
#include "OneButton.h"



#define MAX_LIFE 7
#define NUM_LEDS 7
#define LED_PIN 9
#define BIG_HIT 8
#define LITTLE_HIT 7
#define SETTING_1 3
#define SETTING_2 4
#define BIG_HIT_DAMAGE 3
#define LITTLE_HIT_DAMAGE 1
#define THRESHOLD 60
CRGB leds[NUM_LEDS];

int life = MAX_LIFE;

OneButton bigHitButton;
OneButton littleHitButton;

void bigHitButtonClick(){
  Serial.println("BigHitClick");
  life = life - BIG_HIT_DAMAGE;
}
void littleHitButtonClick(){
  Serial.println("LittleHitClick");
    life = life - LITTLE_HIT_DAMAGE;
}

void setup() 
{
    // No pins to setup, pins can still be used regularly, although it will affect readings

    Serial.begin(115200);
    Serial.println("BattlePoint Life Tracker v1.0");
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
    bigHitButton.setup(BIG_HIT,INPUT_PULLUP, true);
    littleHitButton.setup(LITTLE_HIT,INPUT_PULLUP, true);
    bigHitButton.attachClick(bigHitButtonClick);
    littleHitButton.attachClick(littleHitButtonClick);

    pinMode(SETTING_1, INPUT_PULLUP);
    pinMode(SETTING_2, INPUT_PULLUP);
} 

void set_color(CRGB newColor){
  for (int i=0;i<NUM_LEDS;i++){
    leds[i] = newColor;
  }
}

void blink_leds(int blinks, CRGB fgColor, CRGB bgColor){
  for (int i=0;i<blinks;i++){
    set_color(fgColor);
    FastLED.show();
    FastLED.delay(200);
    set_color(bgColor);
    FastLED.show();
    FastLED.delay(200);    
  }
}

void display_life(){
  set_color(CRGB::Black);
  for(int i=0;i<life;i++){
    leds[i] = CRGB::Green;
  }
}

void loop() 
{
    bigHitButton.tick();
    littleHitButton.tick();
    if ( life <= 0){
      blink_leds(5,CRGB::Red, CRGB::Black);
    }
    else{
      set_color(CRGB::Green);
      display_life();
      FastLED.show();
      FastLED.delay(10);
    }

}