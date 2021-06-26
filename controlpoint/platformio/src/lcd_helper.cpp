#include "lcd_helper.h"
#include <stdio.h>
#include <stdarg.h>
#define SPACE ' '
/**
 * Ok this has some real voodoo in it.
 * __FlashStringHelper is supported by very specific
 * underlying AVR stuff.
 * http://forum.arduino.cc/index.php?topic=293408.0
 * http://forum.arduino.cc/index.php?topic=272313.0
 * 
 * So basically, vsnprintf_P works, but vsnprintf does not.
 * Also, using the F() macro means format string use %S for
 * strings in progmem, and %s for strings NOT in progmem.
 * 
 * 
 */
LcdDisplay::LcdDisplay(SSD1306AsciiWire*  oled){
  _oled = oled;
}

void LcdDisplay::init(){
  _charWidth = _oled->displayWidth() / _oled->charWidth(SPACE);
}
void LcdDisplay::clear(){
  _oled->clear();
}
void LcdDisplay::clearLine(uint8_t lineNum){
  _oled->setRow(lineNum);
  _printSpaces();
}

void LcdDisplay::sprintfLine(uint8_t lineNum, const char* format, ...){  
  char buffer[_charWidth];
  va_list args;
  va_start(args,format);
  vsnprintf_P(buffer, sizeof buffer, format, args);
  va_end(args);
  _oled->setRow(lineNum);
  _oled->print(buffer);
  _printSpaces();
}

void LcdDisplay::sprintfLine(uint8_t lineNum,const __FlashStringHelper *format, ... ){
  char buffer[_charWidth];
  va_list args;
  va_start(args,format);
  vsnprintf_P(buffer, sizeof buffer, (const char*)format, args);
  va_end(args);
  _oled->setRow(lineNum);
  _oled->print(buffer); 
  _printSpaces();
}

void LcdDisplay::_printSpaces(){  
  for ( int i=0;i<_charWidth;i++) _oled->print(SPACE);
  _oled->println(SPACE);
}

void LcdDisplay::printLine(uint8_t lineNum,char* buffer){
  _oled->setRow(lineNum);
  _oled->print(buffer);
  _printSpaces();
}
void LcdDisplay::printLine(uint8_t lineNum,const __FlashStringHelper *data){
  char buffer[_charWidth];
  
  _oled->setRow(lineNum);
  int cursor = 0;
  const char *ptr = (const char *)data;
  while( ( buffer[ cursor ] = pgm_read_byte_near( ptr + cursor ) ) != '\0' ) ++cursor;
  _oled->print(buffer);
  _printSpaces();
}

