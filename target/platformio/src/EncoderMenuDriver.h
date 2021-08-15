#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H
#include "OneButton.h"
#include <menu.h>
#include <ESP32Encoder.h>
class EncoderMenuDriver{
    public:
       EncoderMenuDriver(Menu::navRoot* _nav, ESP32Encoder* _encoder, OneButton* _btn); 
       void update();
       void button_clicked();
       void button_dbl_clicked();       
    private:
        OneButton* button;
        ESP32Encoder* encoder;
        Menu::navRoot* nav;
        long lastEncoderCount;
        boolean clicked;
        boolean dbl_clicked;

        void reset();
};

#endif