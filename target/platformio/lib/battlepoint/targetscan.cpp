#include <Arduino.h>
#include <targetscan.h>
#include <ArduinoLog.h>
#define INITIAL_INTERVAL 1

void _acceptSample(volatile TargetScanner* st, int val ){
    Log.infoln("Sample[%d]= %d", st->_currentSampleIndex, val);
    st->data[st->_currentSampleIndex] = val;    
    if ( st->_currentSampleIndex == (st->numSamples - 1)){
        Log.infoln("Data is Ready:");
        st->dataReady = true;
        st->enableScan = false;
        st->sampling = false;
    }
    st->lastSampleValue = val;
    st->lastScanValue = val;
    st->_currentSampleIndex++;
}

void tick(volatile TargetScanner* st, long millis){
  if ( st->enableScan ){
    if ( st->sampling ){
      int v = st->sampler();
      _acceptSample(st,v);      
    }
    else{
      
      if ( st->_ticksLeftToSample <= 1 ){
        
        int v = st->sampler();        
        if ( v > st->triggerLevel ){
          Log.infoln("Triggered, value=%d", v);
          st->sampling = true;
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
    st->lastScanMillis = millis;
  }
  else{
    Log.traceln("Scan: Ignoring Disabled");
  }
};



bool init(volatile TargetScanner* st, int numSamples, int idleSampleInterval, int triggerLevel, SampleReader sampler){
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
  st->lastScanValue = 0;
  st->lastSampleValue = 0;
  st->dataReady = false;
  st->sampling = false;
};

void enable(volatile TargetScanner* st){
  st->enableScan = true;
  st->dataReady = false;
};

void disable(volatile TargetScanner* st){
  st->enableScan = false;
  st->dataReady = false;
};
