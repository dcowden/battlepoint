#include "display.h"

Display::Display(U8G2* theOled, int newdisplayHeight, int newDisplayWidth, const uint8_t* newFontCode, short newFontWidth, short newFontHeight) {
    oled = theOled;
    displayHeight = newdisplayHeight;
    displayWidth = newDisplayWidth;
    currentLine = 0;
    oled->setFontPosBaseline();
    oled->setFont(newFontCode);
    fontWidth = newFontWidth;
    fontHeight =newFontHeight;    
};

void Display::useFont(const uint8_t* newFontCode, short newFontWidth, short newFontHeight){
    oled->setFontPosBaseline();
    oled->setFont(newFontCode);
    fontWidth = newFontWidth;
    fontHeight =newFontHeight;
};
void Display::startPage(){
    oled->clearBuffer();
    oled->firstPage();
};
void Display::endPage(){
    oled->sendBuffer();
};

void Display::resetLine(){
    currentLine = 0;
};

void Display::newLine(){
    currentLine++;
    oled->setCursor(0,(int)((currentLine * fontHeight) + 1));
};

uint8_t Display::getColumnsWide(){
    return displayWidth/fontWidth;
};

uint8_t Display::getLinesHigh(){
    return displayHeight/fontHeight;
};
