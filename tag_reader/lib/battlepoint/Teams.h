#ifndef __INC_TEAMS_H
#define __INC_TEAMS_H

typedef enum{
    RED=1,
    BLU=2,
    NOBODY=3,
    ALL=4,
    TIE=5  
} Team;

typedef enum{
    COLOR_RED,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_BLACK,
    COLOR_AQUA
} TeamColor;

TeamColor getTeamColor(Team t);
const char* teamTextChar(Team team);
Team oppositeTeam(Team t);
bool isHumanTeam(Team t);
#endif