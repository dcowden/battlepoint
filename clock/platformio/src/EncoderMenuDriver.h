#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H
#include "ClickEncoder.h"
#include <menu.h>
//#include <ESP32Encoder.h>
class EncoderMenuDriver{
    public:
       EncoderMenuDriver(Menu::navRoot* _nav, ClickEncoder* _encoder); 
       void update();
    private:
        ClickEncoder* encoder;
        Menu::navRoot* nav;
        void reset();
};

#endif