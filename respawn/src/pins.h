#ifndef __INC_PINS_H
#define __INC_PINS_H
#define I2C_ADDRESS 0x3C

typedef enum {
    SHORT_DURATION_BTN = 5,
    MEDIUM_DURATION_BTN = 4,
    LONG_DURATION_BTN = 33,
    SOUND = 30,
    TARGET_RIGHT = 32,
    RESPAWN_LEDS = 12,
    I2C_SDA = 21,
    I2C_SCL = 22,
    VBATT=35
} Pins;
#endif