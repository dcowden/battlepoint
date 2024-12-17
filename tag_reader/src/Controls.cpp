#include <Arduino.h>
#include <ArduinoLog.h>
#include "Button2.h"
#include <Pins.h>

Button2 bigHitButton;
Button2 littleHitButton;

void (*little_callback)(void);
void (*big_callback)(void);

void bigHitButtonClick(Button2& b);
void littleHitButtonClick(Button2& b);

void initInputs(){
    pinMode(Pins::BIG_HIT_PIN,INPUT_PULLUP);
    pinMode(Pins::LITTLE_HIT_PIN,INPUT_PULLUP);
    bigHitButton.begin(Pins::BIG_HIT_PIN);
    littleHitButton.begin(Pins::LITTLE_HIT_PIN);
    bigHitButton.setTapHandler(bigHitButtonClick);
    littleHitButton.setTapHandler(littleHitButtonClick);       
}

void updateInputs(){
    bigHitButton.loop();
    littleHitButton.loop();     
}
void registerLittleHitHandler(void (*callback)(void)){
    little_callback = callback;
}
void registerBigHitHandler(void (*callback)(void)){
    big_callback = callback;
};

void bigHitButtonClick(Button2& b){
    big_callback();
}

void littleHitButtonClick(Button2& b){
   little_callback();
}


