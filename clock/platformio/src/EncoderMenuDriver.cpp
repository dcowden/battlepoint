#include "EncoderMenuDriver.h"

EncoderMenuDriver::EncoderMenuDriver(Menu::navRoot* _nav, ClickEncoder* _encoder){
    nav = _nav;
    encoder = _encoder;
}

void EncoderMenuDriver::update(){
    dirty = false;
    //clicks take priority
    int buttonState = encoder->getButton();
    if ( buttonState == ClickEncoder::Clicked ){
        nav->doNav(Menu::enterCmd);
        dirty=true;
    }else if ( buttonState == ClickEncoder::DoubleClicked ){
        nav->doNav(Menu::escCmd);
        dirty=true;
    }
    else{
        long encoderCount = encoder->getValue();
        if (encoderCount < 0 ){
            nav->doNav(Menu::downCmd);
            dirty=true;
        }
        else if ( encoderCount > 0 ){
            nav->doNav(Menu::upCmd);
            dirty=true;
        }
    }
}
