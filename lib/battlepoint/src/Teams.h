#ifndef __INC_TEAMS_H
#define __INC_TEAMS_H
#include <FastLED.h>

typedef enum{
    RED,
    BLU,
    NOBODY,
    ALL  
} Team;

typedef enum{
    RED,
    BLU,
    YELLOW,
    BLACK
} TeamColors;

CRGB getTeamColor(Team t);
char teamTextChar(Team team);
Team oppositeTeam(Team t);
void printTeamText(char* buffer, Team team);
#endif