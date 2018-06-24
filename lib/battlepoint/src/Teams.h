#ifndef __INC_TEAMS_H
#define __INC_TEAMS_H

typedef enum{
    RED,
    BLU,
    NOBODY,
    ALL  
} Team;

typedef enum{
    RED,
    BLUE,
    YELLOW,
    BLACK,
    AQUA
} TeamColor;

TeamColor getTeamColor(Team t);
char teamTextChar(Team team);
Team oppositeTeam(Team t);
void printTeamText(char* buffer, Team team);
#endif