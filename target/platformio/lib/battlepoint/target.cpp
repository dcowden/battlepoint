#include <Arduino.h>
#include <target.h>
#include <targetscan.h>
#include <ArduinoLog.h>


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
  return energy/ (sample_end - sample_start);  
}

double count_average(volatile int* data, int num_samples ){
  double energy = 0;
  for ( int i =0;i<num_samples;i++){
     energy += data[i];
  }
  return energy/ num_samples;   
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
TargetHitData analyze_impact( volatile TargetScanner* scanner, long hit_energy_threshold){
    int num_samples = scanner->numSamples;
    volatile int* data = scanner->data;

    int peak_0 = count_peaks(data,num_samples, 10 , 10);
    int peak_1000 = count_peaks(data,num_samples, 10 , 1000);
    int peak_2000 = count_peaks(data,num_samples, 10 , 2000);
    int peak_3000 = count_peaks(data,num_samples, 10 , 3000);
    int peak_4000 = count_peaks(data,num_samples, 10 , 4000);
    int avg_middle = average_energy_in_interval( data, 46,76); //between 4.5 and 7.5 ms
    int avg_middle2 = average_energy_in_interval( data, 36,86); //between 3.5 and 8.5 ms
    int avg_middle3 = average_energy_in_interval( data, 56,66); //between 5.5 and 6.5 ms
    long avg_middle4 = average_energy_in_interval( data, 30,110); //between 3.0 and 10.0 ms
    long avg_energy= count_average(data,num_samples);


    Serial.print(avg_energy);Serial.print(",");
    Serial.print(avg_middle);Serial.print(",");
    Serial.print(avg_middle2);Serial.print(",");    
    Serial.print(avg_middle3);Serial.print(",");    
    Serial.print(avg_middle4);Serial.print(",");            
    Serial.print(peak_0);Serial.print(",");
    Serial.print(peak_1000);Serial.print(",");
    Serial.print(peak_2000);Serial.print(",");
    Serial.print(peak_3000);Serial.print(",");
    Serial.print(peak_4000);Serial.println("");

}