#include <Arduino.h>
#include <arduinoFFT.h>
#include <target.h>
#include <ArduinoLog.h>

#define FFT_SAMPLES 128
#define ENERGY_FREQ_INDEX_1 4
#define ENERGY_FREQ_INDEX_2 5
#define SAMPLING_FREQUENCY 6000
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

arduinoFFT FFT = arduinoFFT();
FFTData fft_data;

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
        abscissa = ((i * 1.0 * SAMPLING_FREQUENCY) / FFT_SAMPLES);
  break;
    }
    //Serial.print(abscissa, 2);
    //if(scaleType==SCL_FREQUENCY)
    //  Serial.print("Hz");
    //Serial.print(" ");
    Serial.println(vData[i], 2);
  }
  Serial.println();
}

//more sophisticated code could check the two targets
//in parallel, but I dont think that'll be necessary.
//and if it is, we maybe want to use a separate ADC
TargetHitScanResult check_target(int pinReader(void), TargetSettings target,Clock* clock){

    TargetHitScanResult result;
    int targetValue = pinReader();

    if (targetValue >  target.trigger_threshold ){
        for(int i=0;i<FFT_SAMPLES;i++){
            fft_data.vReal[i] = pinReader();
            fft_data.vImag[i] = 0;
        }
        //Serial.println("Data:");
        //PrintVector(fft_data.vReal, FFT_SAMPLES, SCL_TIME);  

        FFT.Windowing(fft_data.vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.Compute(fft_data.vReal, fft_data.vImag, FFT_SAMPLES, FFT_FORWARD);
        FFT.ComplexToMagnitude(fft_data.vReal, fft_data.vImag, FFT_SAMPLES);
        double last_hit_energy = fft_data.vReal[ENERGY_FREQ_INDEX_1] + fft_data.vReal[ENERGY_FREQ_INDEX_2];
        double peak = FFT.MajorPeak(fft_data.vReal, FFT_SAMPLES, SAMPLING_FREQUENCY);
        Log.warningln("TargetScan: e=%d, peak=%d, thresh=%d", last_hit_energy, peak,target.hit_energy_threshold);
        //Serial.println("Computed magnitudes:");
        //PrintVector(fft_data.vReal,(FFT_SAMPLES>>1),SCL_FREQUENCY);
        //Serial.println("Peak Frequency:");
        //
        //Serial.println(x, 6);
        if ( last_hit_energy > target.hit_energy_threshold){
            result.was_sampled = 1;
            result.was_hit = 1;
            result.hit_millis = clock->milliseconds();
            result.last_hit_energy = last_hit_energy;
            result.peak_frequency = peak;
        }        

    }
    else{
        result.was_sampled = 0;
        result.was_hit = 0;
        result.hit_millis = 0;
        result.last_hit_energy = 0;
        result.peak_frequency = 0;
    }
    return result;

}