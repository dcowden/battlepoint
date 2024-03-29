#ifndef __INC_TARGETSCAN_H
#define __INC_TARGETSCAN_H

#include <Arduino.h>
#define MAX_TARGET_SAMPLES 500

typedef int (*SampleReader)(); //for reading a pin
typedef struct {
    int numSamples = 0;
    int data[MAX_TARGET_SAMPLES];
    int _currentSampleIndex = 0;
    int _ticksLeftToSample = 0;
    int idleSampleInterval = 1; //2 means dont sample every other trigger, unless we're scanning
    int triggerLevel = 0;
    SampleReader sampler; //for reading values
    long lastHitMillis = 0;
    long lastScanMillis = 0;
    long sampleTimeMillis = 0;
    int lastScanValue = 0;
    int lastSampleValue = 0;
    bool dataReady = false;
    bool enableScan = false;
    bool sampling = false;
} TargetScanner;

bool initScanner(volatile TargetScanner* st, int numSamples, int idleSampleInterval, int triggerLevel, SampleReader sampler);
bool isReady( volatile TargetScanner* st);
void tick( volatile TargetScanner* st, long millis);
void reset(volatile TargetScanner* st);
void enable(volatile  TargetScanner* st);
void disable(volatile TargetScanner* st);


#endif