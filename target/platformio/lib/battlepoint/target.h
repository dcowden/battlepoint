#ifndef __INC_TARGET_H
#define __INC_TARGET_H
#include <Arduino.h>
#include <targetscan.h>

typedef struct {
    int hits;
    double last_hit_energy;   
} TargetHitData;

TargetHitData analyze_impact( volatile TargetScanner* scanner, long hit_energy_threshold);

#endif