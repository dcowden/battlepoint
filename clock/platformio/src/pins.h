#ifndef __INC_PINS_H
#define __INC_PINS_H
#define I2C_ADDRESS 0x3C

typedef enum {

    DIAL_INDEX = 19,
    LED_TOP=12,
    DIAL_1 = 14,
    DIAL_2 = 27,
    DIAL_3 = 26,
    DIAL_4 = 25,    
    I2C_SDA = 21,
    I2C_SCL = 22,
    ENC_BUTTON =34,
    ENC_DOWN=39,
    ENC_UP=36,
    VBATT=35
} Pins;
#endif