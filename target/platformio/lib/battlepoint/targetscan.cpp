#include <Arduino.h>
#include <targetscan.h>
#include <ArduinoLog.h>
#define INITIAL_INTERVAL 1

void _acceptSample(volatile TargetScanner* st, int val ){
    Log.traceln("Sample[%d]= %d", st->_currentSampleIndex, val);
    st->data[st->_currentSampleIndex] = val;    
    if ( st->_currentSampleIndex == (st->numSamples - 1)){
        long t = millis();
        Log.infoln("Data is Ready, t=%l", t);
        st->dataReady = true;
        st->enableScan = false;
        st->sampling = false;
        st->_currentSampleIndex = 0;
        st->sampleTimeMillis = (long)(t - st->sampleTimeMillis);
    }
    st->lastSampleValue = val;
    st->lastScanValue = val;
    st->_currentSampleIndex++;
}

void tick(volatile TargetScanner* st, long time_millis){
  
  if ( st->enableScan ){
    if ( st->sampling ){
      int v = st->sampler();
      _acceptSample(st,v);      
    }
    else{
      
      if ( st->_ticksLeftToSample <= 1 ){
        
        int v = st->sampler();        
        if ( v > st->triggerLevel ){
          st->sampling = true;
          st->sampleTimeMillis = time_millis;
          _acceptSample(st,v);
        }
        st->lastScanValue = v;
        st->_ticksLeftToSample = st->idleSampleInterval;
      }
      else{           
        st->_ticksLeftToSample--;
        Log.traceln("Scan: %d more ticks till scan", st->_ticksLeftToSample);   
      }
    }
    st->lastScanMillis = time_millis;
  }
  else{
    Log.traceln("Scan: Ignoring Disabled");
  }
  
};

bool initScanner(volatile TargetScanner* st, int numSamples, int idleSampleInterval, int triggerLevel, SampleReader sampler){
  if ( numSamples >= MAX_TARGET_SAMPLES) return false;
  st->numSamples = numSamples;
  st->idleSampleInterval = idleSampleInterval;
  st->triggerLevel = triggerLevel;
  st->sampler = sampler;
  reset(st);
  return true;
}

bool isReady(volatile TargetScanner* st){
  return st->dataReady;
};

void reset(volatile TargetScanner* st){
  st->enableScan = false;
  st->_currentSampleIndex = 0;
  st->_ticksLeftToSample = st->idleSampleInterval;
  st->lastHitMillis = 0;
  st->lastScanMillis = 0;
  st->sampleTimeMillis = 0;
  st->lastScanValue = 0;
  st->lastSampleValue = 0;
  st->dataReady = false;
  st->sampling = false;
};

void enable(volatile TargetScanner* st){
  st->enableScan = true;
  st->sampling = false;
  st->dataReady = false;
  st->lastScanValue = 0;
  st->sampleTimeMillis = 0;
};

void disable(volatile TargetScanner* st){
  st->enableScan = false;
  st->dataReady = false;
};
