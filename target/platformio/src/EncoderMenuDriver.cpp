#include "EncoderMenuDriver.h"

EncoderMenuDriver::EncoderMenuDriver(Menu::navRoot* _nav, ClickEncoder* _encoder){
    nav = _nav;
    encoder = _encoder;
}

void EncoderMenuDriver::update(){
    nav->poll();

    //clicks take priority
    int buttonState = encoder->getButton();
    if ( buttonState == ClickEncoder::Clicked ){
        nav->doNav(Menu::enterCmd);
    }else if ( buttonState == ClickEncoder::DoubleClicked ){
        nav->doNav(Menu::escCmd);
    }
    else{

        long encoderCount = encoder->getValue();
        if (encoderCount < 0 ){
            nav->doNav(Menu::downCmd);
        }
        else if ( encoderCount > 0 ){
            nav->doNav(Menu::upCmd);
        }
    }
    nav->doOutput();
}
