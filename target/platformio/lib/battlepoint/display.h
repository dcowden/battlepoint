#include <Arduino.h>
#include <U8g2lib.h>

typedef struct{
    int fontH;
    int fontW;
} DisplaySettings;

short fontW = 6;
short fontH = 13;

/**
int getLineLocation(int linenum){
  return (int)(linenum * fontH) + 1;
}

void updateDisplay(){  
  oled.clearBuffer();
  oled.sendBuffer();
}
**/
