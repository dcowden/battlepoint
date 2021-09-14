#include <Arduino.h>
#include <target.h>
#include <targetscan.h>
#include <ArduinoLog.h>

int sample_number = 1;

int count_energy(volatile int* data, int num_samples ){
  long energy = 0;
  for ( int i =0;i<num_samples;i++){
     energy += data[i];
  }
  return energy;
}
double average_energy_in_interval(volatile int* data, int sample_start, int sample_end ){
  double energy = 0;
  for ( int i =sample_start;i<sample_end;i++){
     energy += data[i];
  }
  int num_samp = (sample_end - sample_start);
  if ( num_samp > 0){
     return energy/ num_samp;  
  }
  else{
    return 0.0;
  }
  
}

double count_average(volatile int* data, int num_samples ){
  double energy = 0;
  for ( int i =0;i<num_samples;i++){
     energy += data[i];
  }
  if ( num_samples > 0 ){
    return energy/ num_samples;   
  }
  else{
    return 0.0;
  }
  
}

int count_peaks(volatile int* data, int num_samples, int min_thresh, int max_thresh){
  bool climbing = true;
  int peaks=0;
  for ( int i =0;i<num_samples;i++){
    int s = data[i];
    if ( climbing){
      if ( s > max_thresh ){
        climbing = false;
      }
      else{

      }
    }
    else{
      if ( s < min_thresh){
        climbing = true;
        peaks++;
      }
    }
  }
  return peaks;
}


//more sophisticated code could check the two targets
//in parallel, but I dont think that'll be necessary.
//and if it is, we maybe want to use a separate ADC
TargetHitData analyze_impact( volatile TargetScanner* scanner, long hit_energy_threshold,bool printData){

    TargetHitData td;
    td.numSamples = scanner->numSamples;
    td.totalSampleTimeMillis = scanner->sampleTimeMillis;
    td.singleSampleTimeMillis = (double)scanner->sampleTimeMillis / (double)scanner->numSamples;

    volatile int* data = scanner->data;

    td.peak0 = count_peaks(data,td.numSamples, 10 , 10);
    td.peak1000 = count_peaks(data,td.numSamples, 10 , 1000);
    td.peak2000 = count_peaks(data,td.numSamples, 10 , 2000);
    td.peak3000 = count_peaks(data,td.numSamples, 10 , 3000);
    td.peak4000 = count_peaks(data,td.numSamples, 10 , 4000);
    td.middle_energy = average_energy_in_interval( data, 46,76); //between 4.5 and 7.5 ms
    td.middle_energy2 = average_energy_in_interval( data, 36,86); //between 3.5 and 8.5 ms
    td.middle_energy3 = average_energy_in_interval( data, 56,66); //between 5.5 and 6.5 ms
    td.middle_energy4 = average_energy_in_interval( data, 30,110); //between 3.0 and 10.0 ms
    td.avg_energy= count_average(data,td.numSamples);    
    td.last_hit_energy = td.middle_energy2;

    if ( td.middle_energy2 > hit_energy_threshold){
      td.hits =1;
    }
    else{
      td.hits=0;
    }
    return td;

}

void printTargetData( TargetHitData* td){
    Serial.print(td->sample_number);Serial.print(",");
    Serial.print(td->hits);Serial.print(",");
    Serial.print(td->last_hit_energy);Serial.print(",");
    Serial.print(td->avg_energy);Serial.print(",");
    Serial.print(td->middle_energy);Serial.print(",");
    Serial.print(td->middle_energy2);Serial.print(",");
    Serial.print(td->middle_energy3);Serial.print(",");
    Serial.print(td->middle_energy4);Serial.print(",");
    Serial.print(td->peak0);Serial.print(",");
    Serial.print(td->peak1000);Serial.print(",");
    Serial.print(td->peak2000);Serial.print(",");
    Serial.print(td->peak3000);Serial.print(",");
    Serial.print(td->peak4000);Serial.print(","); 
    Serial.print(td->totalSampleTimeMillis);Serial.print(",");
    Serial.print(td->singleSampleTimeMillis);Serial.println("");
}