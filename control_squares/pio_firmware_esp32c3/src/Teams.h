#pragma once

typedef enum{
    RED,
    BLU,
    NOBODY,
    NEUTRAL,
    ALL  
} Team;

typedef enum{
    COLOR_RED,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_BLACK,
    COLOR_GREEN,
    COLOR_AQUA
} TeamColor;

TeamColor getTeamColor(Team t);
char teamTextChar(Team team);
Team oppositeTeam(Team t);
void printTeamText(char* buffer, Team team);
