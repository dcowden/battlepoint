#include <Arduino.h>
#include <target.h>
#include <targetscan.h>
#include <ArduinoLog.h>
#include <TargetClassifier.h>
//#include "tensorflow/lite/version.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

//#define NUM_INPUTS 6
#define NUM_OUTPUTS 1
#define TENSOR_ARENA_SIZE 10*1024

enum TfLiteError {
    TFL_OK,
    TFL_VERSION_MISMATCH,
    TFL_CANNOT_ALLOCATE_TENSORS,
    TFL_NOT_INITIALIZED,
    TFL_INVOKE_ERROR
};

TfLiteError error;
byte tensorArena[TENSOR_ARENA_SIZE] __attribute__((aligned(16)));
tflite::ErrorReporter *reporter;
tflite::MicroInterpreter *interpreter;
TfLiteTensor *input;
TfLiteTensor *output;
const tflite::Model *model;


void setup_target_classifier(){
    static tflite::MicroErrorReporter microReporter;
    static tflite::AllOpsResolver resolver;
    reporter = &microReporter;
    model = tflite::GetModel(target_classifier_quantized_tflite);      

    // assert model version and runtime version match
    if (model->version() != TFLITE_SCHEMA_VERSION) {        
        error = TFL_VERSION_MISMATCH;
        Serial.println("Model provided is schema version %d not equal ");
        reporter->Report(
                "Model provided is schema version %d not equal "
                "to supported version %d.",
                model->version(), TFLITE_SCHEMA_VERSION);

    }    
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensorArena, TENSOR_ARENA_SIZE, reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("CantAllocateTensors");
        error = TFL_CANNOT_ALLOCATE_TENSORS;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

}

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

void classifyModel(TargetHitData*  td , volatile int* data){
  //ml.begin(target_classifier_quantized_tflite);
  //only 10 fields allowed for input max
  //FIELDS=['energy','peak_0','peak_1000','peak_2000','peak_3000','peak_4000','overallAvgEnergy','bin0','bin1','bin2']

  input->data.f[0] = (float)(td->last_hit_energy);
  input->data.f[1] = (float)(td->peak0);
  input->data.f[2] = (float)(td->peak1000);
  input->data.f[3] = (float)(td->peak2000);
  input->data.f[4] = (float)(td->peak3000);
  input->data.f[5] = (float)(td->peak4000);
  input->data.f[6] = (float)(td->overall_avg_energy);
  input->data.f[7] = (float)(td->avg_energy_bins[0]);
  input->data.f[8] = (float)(td->avg_energy_bins[1]);
  input->data.f[9] = (float)(td->avg_energy_bins[2]);


  if (interpreter->Invoke() != kTfLiteOk) {
      error = TFL_INVOKE_ERROR;
      Serial.println("Inference failed");
      reporter->Report("Inference failed");
  }

  float predicted= output->data.f[0];
  td->hitProbability = output->data.f[0];
}

//more sophisticated code could check the two targets
//in parallel, but I dont think that'll be necessary.
//and if it is, we maybe want to use a separate ADC
TargetHitData analyze_impact( volatile TargetScanner* scanner, long hit_energy_threshold,bool printData){

    long start = millis();
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

    classifyModel(&td,data);

    long end = millis() - start;
    Serial.print("analzye impact:");Serial.print(end);Serial.println(" ms");
    //TODO: temporary
    if ( td.peak4000 > 8){
      td.hits =1;
    }
    else{
      td.hits=0;
    }
    return td;

}

void printTargetDataHeaders(){
  Serial.print("sample_no");Serial.print(",");
  Serial.print("side");Serial.print(",");
  Serial.print("hit_prob");Serial.print(",");  
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

void printTargetData( TargetHitData* td, const char side){
    Serial.print(td->sample_number);Serial.print(",");
    Serial.print(side);Serial.print(",");
    //Serial.print(td->hitProbability);Serial.print(",");
    //Serial.printf("%f", td->hitProbability);Serial.print(",");
    Serial.print(td->hitProbability,2);Serial.print(",");
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