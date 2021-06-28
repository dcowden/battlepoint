#include <arduinoFFT.h>

arduinoFFT FFT = arduinoFFT();
int count = 0;
int ANALOG_PIN = 0;
int CHEAT_PIN= 3;
int HIT_PIN=4;
int HIT_INTERVAL_MICROS=100;

int TRIGGER_THRESHOLD= 50;
//const int NUM_SAMPLES = 250;
//int samples[NUM_SAMPLES];
//long times[NUM_SAMPLES];


/*
These values can be changed in order to evaluate the functions
*/
const uint16_t samples = 512; //This value MUST ALWAYS be a power of 2
const double samplingFrequency = 800;


unsigned int sampling_period_us;
unsigned long microseconds;

/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
double vReal[samples];
double vImag[samples];

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03


void setup() {
  sampling_period_us = round(1000000*(1.0/samplingFrequency));
  pinMode(CHEAT_PIN,OUTPUT);
  pinMode(HIT_PIN,OUTPUT);
  digitalWrite(CHEAT_PIN,1);
  digitalWrite(HIT_PIN,1);
  // put your setup code here, to run once:
  //Serial.begin();
  Serial.println("RESET!!!"); 
  Serial.begin(57600);
 
}

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
    if(scaleType==SCL_FREQUENCY)
      Serial.print("Hz");
    Serial.print(" ");
    Serial.println(vData[i], 4);
  }
  Serial.println();
}

void register_hit(int pin){
  digitalWrite(pin,0);
  delayMicroseconds(HIT_INTERVAL_MICROS);
  digitalWrite(pin,1);
}

void fft_sample(){

  /*SAMPLING*/
  microseconds = micros();
  for(int i=0; i<samples; i++)
  {
      vReal[i] = analogRead(ANALOG_PIN);
      vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  }
  
  /* Print the results of the simulated sampling according to time */
  //Serial.println("Data:");
  PrintVector(vReal, samples, SCL_TIME);
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
  if ( vReal[11] > 3000.0 && vReal[12] > 3000.0 ){
    //Serial.println("***** BALL HIT ******");
    register_hit(HIT_PIN);
  }
  else{
    //Serial.println("***CHEATING HIT***");
    register_hit(CHEAT_PIN);
  }
}

void loop() {
  int s = analogRead(ANALOG_PIN);
  if ( s > TRIGGER_THRESHOLD ) { 
    fft_sample();
  }
}
