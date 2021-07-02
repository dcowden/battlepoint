#include <Arduino.h>

#define TARGET_ENERGY_RATIO 0.11
#define TARGET_TRIGGER_THRESHOLD 2000
#define TARGET_HIT_ENERGY_THRESHOLD 20000.0
//signal is over in 60ms
//ESP32 ADC : 6k samples/sec. 128 samples -> 21.3333 ms
//noisy part of the signal was 700hz, about 7 peaks in 20ms
//smooth part was about 116hz
#define TARGET_FFT_SAMPLES 128
#define BP_DEBUG 1

typedef struct {
    int pin;
    int total_hits;
    long last_hit_millis;
    double trigger_threshold;
    double hit_energy_threshold;
} Target;

typedef struct {
    int was_sampled;
    int was_hit;
    double last_hit_energy;   
    long hit_millis; 
    double peak_frequency;
} TargetHitScanResult;

typedef struct {
    double vReal[TARGET_FFT_SAMPLES];
    double vImag[TARGET_FFT_SAMPLES];
} FFTData;

int analog_read(int pin);
TargetHitScanResult check_target(int pinReader(int), FFTData fft_data, Target target);