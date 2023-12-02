#ifndef __INC_PINS_H
#define __INC_PINS_H
#define I2C_ADDRESS 0x3C

typedef enum {
    SHORT_DURATION_BTN = 4,
    MEDIUM_DURATION_BTN = 7,
    LONG_DURATION_BTN = 9,
    SOUND = 11,
    RESPAWN_LEDS = 2,
    I2C_SDA = 21,
    I2C_SCL = 22
} Pins;
#endif