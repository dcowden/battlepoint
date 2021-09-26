#ifndef __INC_TARGET_H
#define __INC_TARGET_H
#include <Arduino.h>
#include <targetscan.h>

#define DATA_BIN_COUNT 20

typedef struct {
    int hits;
    int sample_number;
    double last_hit_energy; 
    double avg_energy_bins[DATA_BIN_COUNT];
    double overall_avg_energy;
    int peak0;
    int peak1000;
    int peak2000;
    int peak3000;
    int peak4000;
    long totalSampleTimeMillis;
    int numSamples;
    float hitProbability;
    double singleSampleTimeMillis;
} TargetHitData;

TargetHitData analyze_impact( volatile TargetScanner* scanner, long hit_energy_threshold,bool printData);
void printTargetData( TargetHitData* td,const char side);
void printTargetDataHeaders();
void setup_target_classifier();
#endif