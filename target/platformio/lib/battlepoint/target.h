#ifndef __INC_TARGET_H
#define __INC_TARGET_H
#include <Arduino.h>
#include <targetscan.h>

typedef struct {
    int hits;
    int sample_number;
    double last_hit_energy; 
    int avg_energy;
    int middle_energy;
    int middle_energy2;
    int middle_energy3;
    int middle_energy4;
    int peak0;
    int peak1000;
    int peak2000;
    int peak3000;
    int peak4000;
    long totalSampleTimeMillis;
    int numSamples;
    double singleSampleTimeMillis;
} TargetHitData;

TargetHitData analyze_impact( volatile TargetScanner* scanner, long hit_energy_threshold,bool printData);
void printTargetData( TargetHitData* td);

#endif