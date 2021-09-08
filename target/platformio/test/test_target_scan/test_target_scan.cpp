#include <Arduino.h>
#include <targetscan.h>
#include <ArduinoLog.h>
#include <unity.h>


#define MOCK_DATA_LENGTH 129
const int SAMPLE_INTERVAL = 3;
const int TRIGGER_THRESH = 100;
const int NUM_SAMPLES = 20;
TargetScanner ts;

int mockData[MOCK_DATA_LENGTH] = {
    11, 50, 12, 0, 13, 6, 117
    ,980 ,1023 ,1024,1025 ,1026 ,703 ,319 ,116 ,12 ,0 ,0 ,0 ,0,0 ,0
    ,0 ,0 ,0 ,333 ,44 ,0 ,1 ,13 ,37 ,336 ,279 ,131 ,0 ,0 ,0 ,0 ,0 ,0 ,0
    ,0 ,0 ,0 ,229 ,79 ,13 ,0 ,0 ,0 ,0 ,0 ,16 ,0 ,0 ,266 ,281 ,154 ,103 ,98
    ,859 ,1010 ,962 ,1023 ,767 ,773 ,404 ,167 ,27 ,0 ,0 ,0 ,0 ,0 ,3 ,20 ,0
    ,6 ,402 ,514 ,650 ,835 ,930 ,1023 ,1023 ,1023 ,616 ,336 ,159 ,122 ,298
    ,488 ,228 ,100 ,28 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,135,113,267
};

int sample_index=0;

void reset_sample_index(){
    sample_index = 0;
}

int mock_adc_reader(){    
    int v = mockData[sample_index++]; 
    Log.infoln("Mock ADC: Returning: %d", v);
    return v;
}

void test_scanner_init(void){
    reset_sample_index();

    init(&ts,NUM_SAMPLES,SAMPLE_INTERVAL,TRIGGER_THRESH,&mock_adc_reader);

    TEST_ASSERT_TRUE_MESSAGE(ts.dataReady == false, "Initially no data ready");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastHitMillis == 0, "Initially no last hit millis");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanMillis == 0, "Initially no last scan millis");
    TEST_ASSERT_TRUE_MESSAGE(ts.sampling == false, "Initially not sampling");
    TEST_ASSERT_TRUE_MESSAGE(ts.enableScan == false , "Initially scanning is disabled");
    TEST_ASSERT_TRUE_MESSAGE(ts.idleSampleInterval == SAMPLE_INTERVAL , "Initially supplied sampling interval");
    TEST_ASSERT_TRUE_MESSAGE(ts._ticksLeftToSample == SAMPLE_INTERVAL , "Initially supplied sampling interval");
    TEST_ASSERT_TRUE_MESSAGE(ts.triggerLevel == TRIGGER_THRESH , "Initially supplied trigger level");
    TEST_ASSERT_TRUE_MESSAGE(ts._currentSampleIndex == 0 , "Initially sample index is zero");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanValue == 0 , "initial  zero");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastSampleValue == 0 , "initial  zero");
}

void test_not_enabled_doesnt_read(void){
    reset_sample_index();

    init(&ts,NUM_SAMPLES,SAMPLE_INTERVAL,TRIGGER_THRESH,&mock_adc_reader);

    tick(&ts,1234);
    TEST_ASSERT_TRUE_MESSAGE(sample_index == 0 , "Not enabled does not sample ");
    TEST_ASSERT_TRUE_MESSAGE(ts._ticksLeftToSample == SAMPLE_INTERVAL , "not reading yet.");
    TEST_ASSERT_TRUE_MESSAGE(ts._currentSampleIndex == 0 , "No samples yet, starts at zero");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanValue == 0 , "still  zero last value");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastSampleValue == 0 , "Should not be sampling yet");

}

void test_enabled_reads(void){
    reset_sample_index();

    init(&ts,NUM_SAMPLES,SAMPLE_INTERVAL,TRIGGER_THRESH,&mock_adc_reader);

    enable(&ts);
    TEST_ASSERT_TRUE_MESSAGE(ts.enableScan == true, "Should  be scanning now ");

    tick(&ts,11111);
    TEST_ASSERT_TRUE_MESSAGE(sample_index == 0 , "Not enough ticks  to sample, need 3, only 1 so far");
    TEST_ASSERT_TRUE_MESSAGE(ts._ticksLeftToSample == 2 , "Should be one tick closer to sampling");

    tick(&ts,22222);
    tick(&ts, 33333);
    TEST_ASSERT_TRUE_MESSAGE(sample_index == 1 , "Should Have Sampled");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanMillis == 33333, "Should Have recent time from tick");    
    TEST_ASSERT_TRUE_MESSAGE(ts._ticksLeftToSample == SAMPLE_INTERVAL , "Should be back to original value ");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanValue ==  11, "Should have fetched the first value");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastSampleValue == 0 , "Should not be sampling yet");
}

void test_sample_interval_one_samples_every_time(void){
    reset_sample_index();
    const int TEST_NUM_SAMPLES = 5;
    init(&ts,TEST_NUM_SAMPLES,1,TRIGGER_THRESH,&mock_adc_reader);
    enable(&ts);
    for ( int i =0;i<TEST_NUM_SAMPLES;i++){
        tick(&ts,i);
    }
    TEST_ASSERT_TRUE_MESSAGE(sample_index == 5 , "Should Have scanned 5 values");
    TEST_ASSERT_TRUE_MESSAGE(ts.sampling == false, "Not sampling yet, first 5 values are below thresdhold");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanValue == 13, "Should Have scanned 5 values");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanMillis == 4, "last clock value incorrect");

    for ( int i =0;i<TEST_NUM_SAMPLES;i++){
        tick(&ts,i);
    }
    TEST_ASSERT_TRUE_MESSAGE(sample_index == 10 , "Should Have scanned 10 values");
    TEST_ASSERT_TRUE_MESSAGE(ts.sampling == true, "Sampling triggered at value 117");
    TEST_ASSERT_TRUE(ts.lastSampleValue == 1024);
    TEST_ASSERT_TRUE(ts._currentSampleIndex == 4);
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanValue == 1024, "Should Have scanned 5 values");
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanMillis == 4, "last clock value incorrect");
}

void test_sample_data_ready(){
    reset_sample_index();
    init(&ts,NUM_SAMPLES,SAMPLE_INTERVAL,TRIGGER_THRESH,&mock_adc_reader);
    enable(&ts);

    //read 33 times. Here's how the sequence should burn through mock data
    //    11, 50, 12, 0, 13, 6, 117
    //    ,980 ,1023 ,1024,1025 ,1026 ,703 ,319 ,116 ,12 ,0 ,0 ,0 ,0,0 ,0
    //        ,0 ,0 ,0 ,333 ,44 ,0 ,1 ,13 ,37 ,336 ,279 ,131 ,0 ,0 ,0 ,0 ,0 ,0 ,0

    // with sample interval 3, it should take 3 ticks to fetch each of: 11, 50, 12, 0, 13, 6, 117
    // 7x3 = 21
    // 117 begins sampling
    // now it should take 20 ticks to get 20 samples
    // and sampling should then stop
    //any number of ticks is ok here, it should ignore ticks after data is ready
    for ( int i=0;i<45;i++){
        tick(&ts,1000 + i );
    }
    TEST_ASSERT_TRUE_MESSAGE(ts._currentSampleIndex == 20 , "Should be ready to read sample 21");
    TEST_ASSERT_TRUE_MESSAGE(sample_index == 26 , "Should be ready to read datapoint 26");
    TEST_ASSERT_TRUE_MESSAGE(ts.sampling == false, "Should have read a full sample set of 20");
    TEST_ASSERT_TRUE(ts.lastSampleValue == 333);
    TEST_ASSERT_TRUE(ts.lastScanValue == 333);
    TEST_ASSERT_TRUE_MESSAGE(ts.lastScanMillis == 1039, "last clock value incorrect");
    TEST_ASSERT_TRUE(ts.dataReady == true);
    TEST_ASSERT_TRUE(ts.data[0] == 117);
    TEST_ASSERT_TRUE(ts.data[1] == 980);
    TEST_ASSERT_TRUE(ts.data[2] == 1023);
    TEST_ASSERT_TRUE(ts.data[3] == 1024);
    TEST_ASSERT_TRUE(ts.data[4] == 1025);
    TEST_ASSERT_TRUE(ts.data[5] == 1026);
    TEST_ASSERT_TRUE(ts.data[19] == 333);   

    //now enable and see if we get a few ticks
    enable(&ts);

    tick(&ts,1000); 
    tick(&ts,1002);
    tick(&ts,1003);
    TEST_ASSERT_TRUE(ts.dataReady == false);
    TEST_ASSERT_TRUE(ts.lastScanValue == 44);
    TEST_ASSERT_TRUE(ts.sampling == false);
    TEST_ASSERT_TRUE(ts.lastScanMillis == 1003 )
}

void setup() {
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    RUN_TEST(test_scanner_init);
    RUN_TEST(test_not_enabled_doesnt_read);
    RUN_TEST(test_enabled_reads);
    RUN_TEST(test_sample_interval_one_samples_every_time);
    RUN_TEST(test_sample_data_ready);
    UNITY_END();
}

void loop() {
    delay(500);
}