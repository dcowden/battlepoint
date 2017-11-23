/**
 * helps rendering text for a 16x2 display
 * Also includes some nice sprintf formatting helpers,
 * So that we can avoid buffers and other annoyting stuff.
 */
#include <LiquidCrystal.h>
#include "Arduino.h"
#include "SSD1306AsciiWire.h"
//#define COLS 17
#ifndef LcdDisplay_cpp
#define LcdDisplay_cpp

class LcdDisplay{
   public:
      LcdDisplay(SSD1306AsciiWire*  oled);
      void init();
      void clearLine(uint8_t lineNum);
      void clear();
      void printLine(uint8_t lineNum, char* buffer);     
      void printLine(uint8_t lineNum,const __FlashStringHelper *buffer);
      void sprintfLine(uint8_t lineNum, const __FlashStringHelper * format, ... );    
      void sprintfLine(uint8_t lineNum, const char* format, ...);

   private: 
     void _printSpaces();
     SSD1306AsciiWire*  _oled;
     uint8_t _charWidth;
};
#endif
