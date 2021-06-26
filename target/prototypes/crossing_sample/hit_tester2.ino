
const int ANALOG_PIN = 0;
const int HIT_PIN=4;
const int HIT_INTERVAL_MICROS=100;
const int TRIGGER_THRESHOLD= 50;
const uint16_t SAMPLE_COUNT = 400; 
const double samplingFrequency = 8000;
const int CROSSING_HIGH=900;
const int CROSSING_LOW=50;
const int CROSSING_THRESHOLD = 7;
unsigned int sampling_period_us;
unsigned long microseconds;

int data[SAMPLE_COUNT];

void setup() {
  sampling_period_us = round(1000000*(1.0/samplingFrequency));
  pinMode(HIT_PIN,OUTPUT);
  digitalWrite(HIT_PIN,1);
  Serial.begin(57600); 
}

void register_hit(){
  digitalWrite(HIT_PIN,0);
  delayMicroseconds(HIT_INTERVAL_MICROS);
  digitalWrite(HIT_PIN,1);
}

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
  microseconds = micros();
  for(int i=0; i<SAMPLE_COUNT; i++)
  {
      data[i] = analogRead(ANALOG_PIN);
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  } 
}

void loop() {
  int s = analogRead(ANALOG_PIN);
  if ( s > TRIGGER_THRESHOLD ) { 
     gather_samples();
     //print_samples();
     int crossing_count = count_crossings();
     if ( crossing_count > CROSSING_THRESHOLD){
         register_hit();
     }
     Serial.println("Crossings: ");
     Serial.print(crossing_count);
     Serial.println(" ");
  }
}
