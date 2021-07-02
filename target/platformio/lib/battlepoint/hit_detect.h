#include <Arduino.h>
#include <arduinoFFT.h>
#define BP_DEBUG 1
const int ANALOG_PIN = 25;
const int TRIGGER_THRESHOLD= 2000;

const double samplingFrequency = 7500;
const double ENERGY_RATIO=0.11;
const double ENERGY_THRESHOLD = 20000.0;
const int CROSSING_HIGH=700;
const int CROSSING_LOW=7;
const int CROSSING_THRESHOLD_LOW = 50;
unsigned int sampling_period_us;
unsigned long elapsed_microseconds;
arduinoFFT FFT = arduinoFFT();
//int data[SAMPLE_COUNT];
const uint16_t samples = 128; //This value MUST ALWAYS be a power of 2
const int count_samples = 70;

int COUNTER = 0;
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
    //Serial.print(abscissa, 2);
    //if(scaleType==SCL_FREQUENCY)
    //  Serial.print("Hz");
    //Serial.print(" ");
    Serial.println(vData[i], 2);
  }
  Serial.println();
}

int is_ball_hit(void){
  //the idea here is that there should be at least a couple of extra peaks
  //in this frequency band.
  //compare with the zero-order frequency so that its relative to the overall
  //strenth of the shot
  //double zero_freq_energy = (double) vReal[0];
  //long total_2nd_harmonic_energy = vReal[3] + vReal[4] + vReal[5] + vReal[6] + vReal[7] + vReal[8] + vReal[9];
  double total_2nd_harmonic_energy = vReal[4] + vReal[5];
  //double energy_ratio = total_2nd_harmonic_energy/zero_freq_energy;
  delay(100); //let this wave finish up
  Serial.print("Energy: ");
  Serial.print(total_2nd_harmonic_energy,2);
  if (  total_2nd_harmonic_energy > ENERGY_THRESHOLD){
    Serial.println("***** BALL HIT ******");    
    return 1;
  }
  else{
    Serial.println("***CHEATING HIT***");
    return 0;
  }
  
  
}

int fft_sample(void){

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
  //Serial.print("Trial:");
  //Serial.println(COUNTER);
  //PrintVector(vReal, (samples >> 1), SCL_FREQUENCY);
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

  //Serial.println(vReal[6]);
  
  return is_ball_hit();
  
}
//a 'peak' is a high value terminated by a value 50% lower than the peak
int count_crossings(void){
  int num_crossings = 0;
  int currentreading = 0;
  int peakreading = 0;
  
  for ( int i=0;i<count_samples;i++){
    currentreading = analogRead(ANALOG_PIN);
    if ( currentreading > peakreading ){
      //riding up
      peakreading = currentreading;
    }
    else{
      if ( currentreading < CROSSING_THRESHOLD_LOW ){
          //found a new low
          num_crossings++;
          peakreading=0;
      }
    }
  }
  Serial.println("Crossings:");
  Serial.println(num_crossings);
  delay(150);
  return 0;
}
/*
void print_samples(){
  for (int i=0;i<SAMPLE_COUNT;i++){
    Serial.print(i);
    Serial.print(" ");
    Serial.println(data[i]);
  }
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
    //Serial.println("Gathering Samples!");
    //gather_samples();  
    COUNTER++;  
    return  fft_sample();
    //return count_crossings();
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
  else{
    return 0;
  }

}

