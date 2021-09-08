#include <Arduino.h>
#include <arduinoFFT.h>
#include <target.h>
#include <ArduinoLog.h>
//#include <driver/adc.h>
//#include <soc/sens_reg.h>
//#include <soc/sens_struct.h>
#include <Yin.h>

#define ENERGY_FREQ_INDEX_1 4
#define ENERGY_FREQ_INDEX_2 5
#define SAMPLING_FREQUENCY 10000
#define MIN_DURATION_MILLIS 100
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

arduinoFFT FFT = arduinoFFT();
FFTData fft_data;
int16_t yin_samples[TARGET_FFT_SAMPLES];


unsigned int sampling_period_us;
unsigned long microseconds;
unsigned long start_micros;
int sample_number = 1;

void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    double abscissa;
    /* Print abscissa value */
    switch (scaleType)
    {
      case SCL_INDEX:
        abscissa = (i * 1.0);
  break;
      case SCL_TIME:
        abscissa = ((i * 1.0) / SAMPLING_FREQUENCY);
  break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * SAMPLING_FREQUENCY) / TARGET_FFT_SAMPLES);
  break;
    }
    Serial.print(abscissa, 4);
    if(scaleType==SCL_FREQUENCY)
      Serial.print("Hz");
    Serial.print(" ");
    Serial.println(vData[i], 2);
  }
  Serial.println();
}

TargetHitScanResult empty_target_scan(){
  TargetHitScanResult r;
  r.was_sampled = 0;
  r.was_hit = 0;
  r.hit_millis = 0;
  r.last_hit_energy = 0;
  r.peak_frequency = 0;   
  return r;

}
Yin yin;

int count_energy( int16_t* data, int num_samples ){
  long energy = 0;
  for ( int i =0;i<num_samples;i++){
     energy += data[i];
  }
  return energy;
}
double average_energy_in_interval(int16_t* data, int sample_start, int sample_end ){
  double energy = 0;
  for ( int i =sample_start;i<sample_end;i++){
     energy += data[i];
  }
  return energy/ (sample_end - sample_start);  
}

double count_average(int16_t* data, int num_samples ){
  double energy = 0;
  for ( int i =0;i<num_samples;i++){
     energy += data[i];
  }
  return energy/ num_samples;   
}

int count_peaks( int16_t* data, int num_samples, int min_thresh, int max_thresh){
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
      else{

      }
    }
  }
  return peaks;
}

//more sophisticated code could check the two targets
//in parallel, but I dont think that'll be necessary.
//and if it is, we maybe want to use a separate ADC
void check_target(int pinReader(void), TargetHitScanResult* result, TargetSettings target,Clock* clock){

    sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));
    //Log.infoln("Pin Value: %d/%l", targetValue, target.trigger_threshold);
    //if (targetValue > target.trigger_threshold ){
    long microseconds = micros();  
    long start_millis = millis();
    start_micros = micros();
    for(int i=0;i<TARGET_FFT_SAMPLES;i++){
      int t = pinReader();
      fft_data.vReal[i] = t;      
      yin_samples[i] = t;
      //fft_data.vReal[i] = adc1_get_raw(ADC1_CHANNEL_7);
      //fft_data.vReal[i] = read_adc();
      fft_data.vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
      //  //empty loop
      }
      microseconds += sampling_period_us;
    }
    
    
    //Serial.print("Samples collected in "); Serial.print((micros()-start_micros)/1000);Serial.println(" ms");
    long fft_start = micros();
    //Serial.println("Data:");
    //PrintVector(fft_data.vReal, TARGET_FFT_SAMPLES, SCL_TIME);  
    //int peaks = find_peaks(500,3500,&fft_data);
    //Serial.print("Peaks Found: ");    Serial.println(peaks);

    
    FFT.Windowing(fft_data.vReal, TARGET_FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(fft_data.vReal, fft_data.vImag, TARGET_FFT_SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(fft_data.vReal, fft_data.vImag, TARGET_FFT_SAMPLES);

    //double last_hit_energy = fft_data.vReal[ENERGY_FREQ_INDEX_1] + fft_data.vReal[ENERGY_FREQ_INDEX_2];
    //double last_hit_energy = 0;
    //for ( int i=22;i<=35;i++){
    //  last_hit_energy += fft_data.vReal[i];
    //}
    //last_hit_energy = last_hit_energy / (35-22);

    long simple_start = millis();
    long energy = count_energy(yin_samples,TARGET_FFT_SAMPLES);
    

    int peak_0 = count_peaks(yin_samples,TARGET_FFT_SAMPLES, 10 , 10);
    int peak_1000 = count_peaks(yin_samples,TARGET_FFT_SAMPLES, 10 , 1000);
    int peak_2000 = count_peaks(yin_samples,TARGET_FFT_SAMPLES, 10 , 2000);
    int peak_3000 = count_peaks(yin_samples,TARGET_FFT_SAMPLES, 10 , 3000);
    int peak_4000 = count_peaks(yin_samples,TARGET_FFT_SAMPLES, 10 , 4000);
    int avg_middle = average_energy_in_interval( yin_samples, 46,76); //between 4.5 and 7.5 ms
    int avg_middle2 = average_energy_in_interval( yin_samples, 36,86); //between 3.5 and 8.5 ms
    int avg_middle3 = average_energy_in_interval( yin_samples, 56,66); //between 5.5 and 6.5 ms
    long avg_middle4 = average_energy_in_interval( yin_samples, 30,110); //between 3.0 and 10.0 ms
    long avg_energy= count_average(yin_samples,TARGET_FFT_SAMPLES);


    //Serial.print("Simple Calcs in ");Serial.print(millis() - simple_start);Serial.println(" ms");

    //Log.infoln("TargetScan: energy=%F, peak=%F, thresh=%l", last_hit_energy, peak,target.hit_energy_threshold);
    //Serial.println("Computed magnitudes:");
    //PrintVector(fft_data.vReal,(TARGET_FFT_SAMPLES>>1),SCL_FREQUENCY);
    double fft_peak = FFT.MajorPeak(fft_data.vReal, TARGET_FFT_SAMPLES, SAMPLING_FREQUENCY);    
    //Serial.print("FFT compute:: "); Serial.print((micros()-fft_start)/1000);Serial.println(" ms");
    long yin_start = micros();
    Yin_init(&yin, TARGET_FFT_SAMPLES, 1.0);
    long yin_peak = Yin_getPitch(&yin, yin_samples);
    float yin_prob = Yin_getProbability(&yin);

    /**
    Serial.print("YIN compute:: "); Serial.print((micros()-yin_start)/1000);Serial.println(" ms");
    Serial.print("Peak Frequency--> FFT="); Serial.print(fft_peak);Serial.print(" ,YIN=");Serial.print(yin_peak);    
    Serial.print(" [prob]=");Serial.println(yin_prob);
    Serial.print("Simple: energy=");Serial.print(energy);Serial.print(", avg=");Serial.println(avg_energy);
    Serial.print("Energy 4.5-7.5ms=");Serial.println(avg_middle);
    Serial.print("Peaks_1000=");Serial.println(peak_1000);
    Serial.print("Peaks_2000=");Serial.println(peak_2000);
    Serial.print("Peaks_3000=");Serial.println(peak_3000);
    Serial.print("Peaks_4000=");Serial.println(peak_4000);
    **/
    Serial.print(sample_number);Serial.print(",");
    Serial.print(fft_peak);Serial.print(",");
    Serial.print(yin_peak);Serial.print(",");
    Serial.print(energy);Serial.print(",");
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

    //Serial.print("Avg Energy:"); Serial.println(last_hit_energy);

    /**   
    if ( peaks > 2 ){
        result->was_sampled = 1;
        result->was_hit = 1;
        result->hit_millis = clock->milliseconds();
        result->last_hit_energy = peaks;
        //result->peak_frequency = peak;
        Log.warningln("HIT! %F > %l", peaks, target.hit_energy_threshold);
    }  
    **/
    //Serial.print("check_target: "); Serial.print((micros()-start_micros)/1000);Serial.println(" ms");
    //Log.traceln("Result Was Hit=%d",result->was_hit);
    //wait till its been long enough so we dont double-sample
    long t = millis();
    while ( (millis() - start_millis) < MIN_DURATION_MILLIS ){ }
    //Serial.print("<waited ");Serial.print(millis()-t );Serial.print(" more.>");
    sample_number++;
}