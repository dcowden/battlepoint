
#define I2C_ADDRESS 0x3C

typedef enum {
    TARGET_LEFT = 25,
    TARGET_RIGHT = 26,
    LED_LEFT_EDGE = 12,
    LED_RIGHT_EDGE = 14,
    LED_CENTER_VERTICAL = 15,
    LED_TOP = 16,
    LED_BOTTOM = 17,
    MOUSE_UP= 36,
    MOUSE_DOWN = 34,
    MOUSE_CENTER=35,
    I2C_SDA = 21,
    I2C_SCL = 22
} Pins;