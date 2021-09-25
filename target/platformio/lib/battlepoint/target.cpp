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

void populate_energy_bins(TargetHitData*  td , volatile int* data){
  const int bin_size=td->numSamples/ DATA_BIN_COUNT;
  int bin_counter=0;
  double total_energy= 0.0;
  double overall_total_energy=0.0;
  for ( int i=0;i<td->numSamples;i++){
      //Serial.print("v=");Serial.print(data[i]);Serial.print("  ");
      overall_total_energy += data[i];
      total_energy += data[i];
      if ( (i % bin_size) == 0  ){          
          td->avg_energy_bins[bin_counter] = total_energy/(double)bin_size;
          //Serial.print("bin");Serial.print(bin_counter);Serial.print("=");Serial.println(td->avg_energy_bins[bin_counter]);
          total_energy=0.0; 
          bin_counter++;
      }
  }
  td->overall_avg_energy = total_energy / (double)td->numSamples;
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
    td.sample_number = sample_number++;
    td.numSamples = scanner->numSamples;
    td.totalSampleTimeMillis = scanner->sampleTimeMillis;
    td.singleSampleTimeMillis = (double)scanner->sampleTimeMillis / (double)scanner->numSamples;

    volatile int* data = scanner->data;

    td.peak0 = count_peaks(data,td.numSamples, 10 , 10);
    td.peak1000 = count_peaks(data,td.numSamples, 10 , 1000);
    td.peak2000 = count_peaks(data,td.numSamples, 10 , 2000);
    td.peak3000 = count_peaks(data,td.numSamples, 10 , 3000);
    td.peak4000 = count_peaks(data,td.numSamples, 10 , 4000);
    populate_energy_bins(&td, data);
    td.last_hit_energy = td.overall_avg_energy;

    //TODO: temporary
    if ( td.peak4000 > 6){
      td.hits =1;
    }
    else{
      td.hits=0;
    }
    return td;

}

void printTargetDataHeaders(){
  Serial.print("sample_no");Serial.print(",");
  Serial.print("hits");Serial.print(",");
  Serial.print("energy");Serial.print(",");
  Serial.print("peak_0");Serial.print(",");
  Serial.print("peak_1000");Serial.print(",");
  Serial.print("peak_2000");Serial.print(",");
  Serial.print("peak_3000");Serial.print(",");
  Serial.print("peak_4000");Serial.print(",");  
  Serial.print("totalSampleTime[ms]");Serial.print(",");  
  Serial.print("avgSampleTime[ms]");Serial.print(",");  
  Serial.print("overallAvgEnergy");Serial.print(",");  
  for ( int i=0;i<DATA_BIN_COUNT;i++){
    Serial.print("bin");Serial.print(i);
    Serial.print(",");
  }
  Serial.println("");
}

void printTargetData( TargetHitData* td){
    Serial.print(td->sample_number);Serial.print(",");
    Serial.print(td->hits);Serial.print(",");
    Serial.print(td->last_hit_energy);Serial.print(",");
    Serial.print(td->peak0);Serial.print(",");
    Serial.print(td->peak1000);Serial.print(",");
    Serial.print(td->peak2000);Serial.print(",");
    Serial.print(td->peak3000);Serial.print(",");
    Serial.print(td->peak4000);Serial.print(","); 
    Serial.print(td->totalSampleTimeMillis);Serial.print(",");
    Serial.print(td->singleSampleTimeMillis);Serial.print(",");  
    Serial.print(td->overall_avg_energy);Serial.print(",");    
    for ( int i=0;i<DATA_BIN_COUNT;i++){
      Serial.print(td->avg_energy_bins[i]);
      Serial.print(",");
    }
    Serial.println("");
}