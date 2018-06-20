#ifndef __INC_TEAMS_H
#define __INC_TEAMS_H
#include <FastLED.h>

typedef enum{
    RED,
    BLU,
    NOBODY,
    ALL  
} Team;

CRGB getTeamColor(Team t);
char teamTextChar(Team team);
void printTeamText(char* buffer, Team team);
#endif