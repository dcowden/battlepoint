#include <Arduino.h>
#include <arduinoFFT.h>
#define BP_DEBUG 1
const int ANALOG_PIN = 25;
const int TRIGGER_THRESHOLD= 2000;

const double samplingFrequency = 2000;
const int CROSSING_HIGH=700;
const int CROSSING_LOW=7;
const int CROSSING_THRESHOLD = 5;
unsigned int sampling_period_us;
unsigned long elapsed_microseconds;
arduinoFFT FFT = arduinoFFT();
//int data[SAMPLE_COUNT];
const uint16_t samples = 128; //This value MUST ALWAYS be a power of 2

double vReal[samples];
double vImag[samples];

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

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
        abscissa = ((i * 1.0) / samplingFrequency);
  break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * samplingFrequency) / samples);
  break;
    }
    Serial.print(abscissa, 6);
    //if(scaleType==SCL_FREQUENCY)
    //  Serial.print("Hz");
    Serial.print(" ");
    Serial.println(vData[i], 2);
  }
  Serial.println();
}

int fft_sample(){

  /*SAMPLING*/
  elapsed_microseconds = micros();
  for(int i=0; i<samples; i++)
  {
      vReal[i] = analogRead(ANALOG_PIN);
      vImag[i] = 0;
      while(micros() - elapsed_microseconds < sampling_period_us){
        //empty loop
      }
      elapsed_microseconds += sampling_period_us;
  }
  
  /* Print the results of the simulated sampling according to time */
  //Serial.println("Data:");
  //PrintVector(vReal, samples, SCL_TIME);
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  /* Weigh data */
  //Serial.println("Weighed data:");
  //PrintVector(vReal, samples, SCL_TIME);
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  //Serial.println("Computed Real values:");
  //PrintVector(vReal, samples, SCL_INDEX);
  //Serial.println("Computed Imaginary values:");
  //PrintVector(vImag, samples, SCL_INDEX);
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
  //Serial.println("Computed magnitudes:");
  PrintVector(vReal, (samples >> 1), SCL_FREQUENCY);
  //Serial.println("Peak Frequency:");
  //double x = FFT.MajorPeak(vReal, samples, samplingFrequency);
  //Serial.println(x, 6);
  //Serial.print("Slot 11: ");
  //Serial.println(vReal[11],4);
  //Serial.print("Slot 12: ");
  //Serial.println(vReal[12],4);  

  //Serial.print(vReal[11]);
  //Serial.print(" ");
  //Serial.println(vReal[12]);
  Serial.print("203Hz: ");
  Serial.println(vReal[6]);
  if ( vReal[6] > 4000.0  ){
    Serial.println("***** BALL HIT ******");
    return 1;
  }
  else{
    Serial.println("***CHEATING HIT***");
    return 0;
  }
  
}

/*
void print_samples(){
  for (int i=0;i<SAMPLE_COUNT;i++){
    Serial.print(i);
    Serial.print(" ");
    Serial.println(data[i]);
  }
}

int count_crossings(){
    int crossing_count = 0;
    boolean in_spike = false;
    for ( int i=0;i<SAMPLE_COUNT;i++){
      int current_sample = data[i];
        if (in_spike){
           if ( current_sample < CROSSING_LOW ){
              in_spike = false;
           }
        }
        else{
          if ( current_sample > CROSSING_HIGH ){
            in_spike = true;
            crossing_count++;
          }
        }
    }
    return crossing_count;
}

void gather_samples(){
  elapsed_microseconds = micros();
  sampling_period_us = round(1000000*(1.0/samplingFrequency));
  for(int i=0; i<SAMPLE_COUNT; i++)
  {
      data[i] = analogRead(ANALOG_PIN);
      while(micros() - elapsed_microseconds < sampling_period_us){
        //empty loop
      }
      elapsed_microseconds += sampling_period_us;
  } 
}*/

int poll_for_hit() {
  int s = analogRead(ANALOG_PIN);

  if ( s > TRIGGER_THRESHOLD ) { 
    Serial.println("Gathering Samples!");
    //gather_samples();    
    return  fft_sample();
    /**
    int crossing_count = count_crossings();
    bool is_hit = ( crossing_count > CROSSING_THRESHOLD);    
    
    print_samples();
    Serial.println("Crossings: ");
    Serial.print(crossing_count);
    Serial.println(" ");
    Serial.print("Hit=");
    Serial.println(is_hit);
    **/
  }

}

