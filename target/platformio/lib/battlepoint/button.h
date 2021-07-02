#ifndef __INC_BUTTON_H
#define __INC_BUTTON_H
#include <Arduino.h>
#include <Clock.h>

typedef struct{
    bool is_pressed;
    long last_update_millis;
    long held_millis;
} ButtonState;

ButtonState update(int currentPinValue, ButtonState oldState, int pressedValue, Clock* clock ){
    ButtonState newState;

    long current_millis = clock->milliseconds();
    long time_since_last_update = current_millis - oldState.last_update_millis;
    bool is_currently_pressed = (currentPinValue == pressedValue);

    newState.last_update_millis = current_millis;
    newState.is_pressed = is_currently_pressed;
    if ( is_currently_pressed ){
        newState.held_millis = oldState.held_millis + time_since_last_update;
    }
    else{
        newState.held_millis = 0;
    }
    return newState;
}
#endif