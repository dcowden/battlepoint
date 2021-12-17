#ifndef __INC_PINS_H
#define __INC_PINS_H
#define I2C_ADDRESS 0x3C

typedef enum {

    DIAL_INDEX = 5,
    LED_TOP=19,
    DIAL_1 = 12, //14
    DIAL_2 = 15, //27
    DIAL_3 = 2, //26
    DIAL_4 = 25,    
    I2C_SDA = 21,
    I2C_SCL = 22,
    ENC_BUTTON =34,
    ENC_DOWN=39,
    ENC_UP=36,
    VBATT=35
} Pins;
#endif