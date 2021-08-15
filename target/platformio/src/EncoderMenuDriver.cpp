#include "EncoderMenuDriver.h"

EncoderMenuDriver::EncoderMenuDriver(Menu::navRoot* _nav, ESP32Encoder* _encoder, OneButton* _btn){
    nav = _nav;
    encoder = _encoder;
    button = _btn;
}

void EncoderMenuDriver::update(){
    nav->poll();
    button->tick();
    //clicks take priority
    if ( clicked ){
        nav->doNav(Menu::enterCmd);
        clicked = false;
    }else if ( dbl_clicked){
        nav->doNav(Menu::escCmd);
        dbl_clicked = false;
    }
    else{
        long encoderCount = encoder->getCount();
        if (encoderCount > lastEncoderCount ){
            nav->doNav(Menu::downCmd);
            lastEncoderCount = encoderCount;
        }
        else if ( encoderCount < lastEncoderCount){
            nav->doNav(Menu::upCmd);
            lastEncoderCount = encoderCount;
        }
    }
    nav->doOutput();
}

void EncoderMenuDriver::button_clicked(){
    clicked = true;
}

void EncoderMenuDriver::button_dbl_clicked(){
    dbl_clicked = true;
}
