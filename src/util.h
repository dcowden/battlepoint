
#include <FastLED.h>
#include <ESP8266WiFi.h>
#ifndef BP_UTIL_H
#define BP_UTIL_H
#define RED 1
#define BLU 2
#define NOBODY 3
#define BOTH 4
#define GAME_MODE_KOTH 0
#define GAME_MODE_AD 1
#define GAME_MODE_CP 2

void print_team(uint8_t team);
char* team_text(uint8_t team );
char* game_mode_text(char* buff, uint8_t gameMode);
char team_text_char( uint8_t team);
void print_team_text(char* buffer, uint8_t team);
void print_team_text_short(char* buffer, uint8_t team);
void print_game_mode_text(char* buffer, uint8_t mode);
CRGB get_team_color(uint8_t team);
#endif
