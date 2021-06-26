#ifndef __INC_GAME_DISPLAY_H
#define __INC_GAME_DISPLAY_H
#include <Game.h>
#include <Arduino.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

class GameDisplay {
    public:
        GameDisplay( SSD1306AsciiWire*  display, Game* game);
        void update();

    protected:
        Game* _game;
        SSD1306AsciiWire* _display;
};
#endif
