#ifndef __INC_GAMEMETER_H
#define __INC_GAMEMETER_H
#include <Arduino.h>
#include <LedMeter.h>

#define STANDARD_METER_MAX_VAL 10

typedef struct{
    LedMeter* leftTop;
    LedMeter* leftBottom;
    LedMeter* rightTop;
    LedMeter* rightBottom;
    LedMeter* center;
    LedMeter* left;
    LedMeter* right;
} MeterSettings;


void updateLeds(MeterSettings* meters, long current_time_millis );
void setFlashMeterForTeam(Team t, MeterSettings* meters, FlashInterval fi );
void setMetersToFlashInterval(MeterSettings* meters, long flashInterval);
void setHorizontalMetersToTeamColors(MeterSettings* meters);
void setHorizontalMetersToNeutralColors(MeterSettings* meters);

#endif