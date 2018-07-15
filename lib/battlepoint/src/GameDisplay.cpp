#include <GameDisplay.h>

GameDisplay::GameDisplay( SSD1306AsciiWire*  display, Game* game){
    _game = game;
    _display = display;    
};

void GameDisplay::update(){
    //do nothing for now;
};