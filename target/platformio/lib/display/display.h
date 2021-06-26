#include <Arduino.h>
#include <U8g2lib.h>


class Display {
    public:
        Display(U8G2* theOled, int newdisplayHeight, int newDisplayWidth, const uint8_t* newFontCode, short newFontWidth, short newFontHeight);
        void clear();
        void useFont(const uint8_t* newFontCode, short newFontWidth, short newFontHeight);
        void resetLine();
        void newLine();
        void startPage();
        void endPage();
        uint8_t getColumnsWide();
        uint8_t getLinesHigh();
        U8G2* oled;
        int displayHeight;
        int displayWidth;
        int currentLine;
        int fontHeight;
        int fontWidth;
};