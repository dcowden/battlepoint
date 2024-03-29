#ifndef __INC_PINS_H
#define __INC_PINS_H
#define I2C_ADDRESS 0x3C

typedef enum {
    DF_PLAYER_TX = 5,
    DF_PLAYER_RX = 4,
    TARGET_LEFT = 33,
    TARGET_RIGHT = 32,
    LED_LEFT_EDGE = 12,
    LED_RIGHT_EDGE = 13,
    LED_CENTER_VERTICAL = 14,
    LED_TOP = 27,
    LED_BOTTOM = 26,
    I2C_SDA = 21,
    I2C_SCL = 22,
    ENC_BUTTON =34,
    ENC_DOWN=39,
    ENC_UP=36,
    VBATT=35
} Pins;
#endif


/**
 *     TARGET_LEFT = 25,
    TARGET_RIGHT = 39,
    LED_LEFT_EDGE = 12,
    LED_RIGHT_EDGE = 14,
    LED_CENTER_VERTICAL = 15,
    LED_TOP = 16,
    LED_BOTTOM = 17,
    MOUSE_UP= 34,
    MOUSE_DOWN = 36,
    MOUSE_CENTER=39,
    I2C_SDA = 21,
    I2C_SCL = 22,
    ENC_BUTTON =33,
    ENC_DOWN=32,
    ENC_UP=34,
    VBATT=35
**/