#include <Arduino.h>
#include <arduinoFFT.h>
#include <target.h>
#include <ArduinoLog.h>
#include <driver/adc.h>
#include <soc/sens_reg.h>
#include <soc/sens_struct.h>

#define ENERGY_FREQ_INDEX_1 4
#define ENERGY_FREQ_INDEX_2 5
#define SAMPLING_FREQUENCY 1000
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

arduinoFFT FFT = arduinoFFT();
FFTData fft_data;
unsigned int sampling_period_us;
unsigned long microseconds;
unsigned long start_micros;


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


//more sophisticated code could check the two targets
//in parallel, but I dont think that'll be necessary.
//and if it is, we maybe want to use a separate ADC
void check_target(int pinReader(void), TargetHitScanResult* result, TargetSettings target,Clock* clock){
    microseconds = micros();
    start_micros = micros();
    sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));

    //Log.infoln("Pin Value: %d/%l", targetValue, target.trigger_threshold);
    //if (targetValue > target.trigger_threshold ){

    long start_micros = micros();
    for(int i=0;i<TARGET_FFT_SAMPLES;i++){
      fft_data.vReal[i] = pinReader();      
      //fft_data.vReal[i] = adc1_get_raw(ADC1_CHANNEL_7);
      //fft_data.vReal[i] = read_adc();
      fft_data.vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
      //  //empty loop
      }
      microseconds += sampling_period_us;
    }

    Serial.print("Samples collected in "); Serial.print((micros()-start_micros)/1000);Serial.println(" ms");
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


    //Log.infoln("TargetScan: energy=%F, peak=%F, thresh=%l", last_hit_energy, peak,target.hit_energy_threshold);
    Serial.println("Computed magnitudes:");
    //PrintVector(fft_data.vReal,(TARGET_FFT_SAMPLES>>1),SCL_FREQUENCY);
    double peak = FFT.MajorPeak(fft_data.vReal, TARGET_FFT_SAMPLES, SAMPLING_FREQUENCY);    
    Serial.print("Peak Frequency:"); Serial.println(peak);
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
    Serial.print("check_target: "); Serial.print((micros()-start_micros)/1000);Serial.println(" ms");
    Log.traceln("Result Was Hit=%d",result->was_hit);

}