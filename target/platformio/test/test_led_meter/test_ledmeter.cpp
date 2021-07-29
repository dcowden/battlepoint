#include <Arduino.h>
#include <LedMeter.h>
#include <unity.h>
#include <FastLED.h>
#include <ArduinoLog.h>
#define LED_COUNT 8


CRGB testLeds[LED_COUNT];

CRGB ALL_BLUE[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                            CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue };

CRGB ALL_BLACK[LED_COUNT] = {CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black,
                            CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black };

void resetLEDS(){    
    for ( int i=0;i<LED_COUNT;i++){
        testLeds[i] = CRGB::Black;
    }
}

LedMeter simpleMeter { 
    .startIndex=0, 
    .endIndex=7, 
    .leds = testLeds,    
    .max_val=100, 
    .val=0, 
    .flash_interval_millis=FlashInterval::FLASH_NONE, 
    .fgColor=CRGB::Blue, 
    .bgColor=CRGB::Black };  //8 lights

LedMeter reversedMeter { 
    .startIndex=7, 
    .endIndex=0, 
    .leds = testLeds,     
    .max_val=100,
    .val=0,
    .flash_interval_millis=FlashInterval::FLASH_NONE, 
    .fgColor=CRGB::Blue, 
    .bgColor=CRGB::Black }; //8 lights

LedMeter subsetMeter { 
    .startIndex=0, 
    .endIndex=3, 
    .leds = testLeds,     
    .max_val=100,
    .val=0, 
    .flash_interval_millis=FlashInterval::FLASH_NONE,      
    .fgColor=CRGB::Blue, 
    .bgColor=CRGB::Black }; //4 lights, first half

void assert_leds_equal(CRGB* expected){
    for (int i=0;i<LED_COUNT;i++,expected++){
        //Serial.print("LED ");Serial.print(i);Serial.print(": ");Serial.print(*expected);Serial.print("--");Serial.println(testLeds[i]);
        TEST_ASSERT_EQUAL(*expected,testLeds[i]);
    }
}

void meterAssertVal(LedMeter meter, int val, CRGB* expectedVals){
    resetLEDS();
    meter.val = val;
    updateLedMeter(meter);
    assert_leds_equal(expectedVals);
}

void test_meter_initially_all_black(void) {
    resetLEDS();
    assert_leds_equal(ALL_BLACK);
}

void test_basic_meter_zero(void){
    meterAssertVal(simpleMeter,0,ALL_BLACK);
}

void test_basic_meter_max_value(void){
    meterAssertVal(simpleMeter,100,ALL_BLUE);
}

void test_basic_meter_mid_value(void){

    CRGB expected[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue,CRGB::Blue,
                                CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black };
    meterAssertVal(simpleMeter,50,expected);
}

void test_basic_meter_nearly_full_value_still_isnt_full(void){
    CRGB expected[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                                CRGB::Blue,CRGB::Blue,CRGB::Blue, CRGB::Black };
    meterAssertVal(simpleMeter,97,expected);
}

void test_basic_meter_tiny_value_still_isnt_lit(void){
    meterAssertVal(simpleMeter,5,ALL_BLACK);
}

void test_reversed_meter_zero(void){
    meterAssertVal(reversedMeter,0,ALL_BLACK);   
}

void test_reversed_meter_max_value(void){
    meterAssertVal(reversedMeter,100,ALL_BLUE);
}

void test_reversed_meter_nearly_full_value_still_isnt_full(void){
    CRGB expected[LED_COUNT] = {CRGB::Black, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                                CRGB::Blue,CRGB::Blue,CRGB::Blue, CRGB::Blue };
    meterAssertVal(reversedMeter,97,expected);
}

void test_reversed_meter_tiny_value_still_isnt_lit(void){
    meterAssertVal(reversedMeter,5,ALL_BLACK);
}

void test_reversed_meter_mid_value(void){
    CRGB expected[LED_COUNT] = { CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black, 
                                CRGB::Blue,CRGB::Blue,CRGB::Blue,CRGB::Blue };
    meterAssertVal(reversedMeter,50,expected);
}

void test_subset_meter_min_value(void){  
    meterAssertVal(subsetMeter,0,ALL_BLACK);
}

void test_subset_meter_max_value(void){
    CRGB expected[LED_COUNT] = { CRGB::Blue,CRGB::Blue, CRGB::Blue, CRGB::Blue, 
                                CRGB::Black,CRGB::Black,CRGB::Black,CRGB::Black };    
    meterAssertVal(subsetMeter,100,expected);
}

void test_subset_meter_mid_value(void){
    CRGB expected[LED_COUNT] = { CRGB::Blue,CRGB::Blue, CRGB::Black, CRGB::Black, 
                                CRGB::Black,CRGB::Black,CRGB::Black,CRGB::Black };    
    meterAssertVal(subsetMeter,50,expected);
}


//TODO: add tests for updateController
void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();

    //simple meter tests
    RUN_TEST(test_meter_initially_all_black);
    RUN_TEST(test_basic_meter_zero);
    RUN_TEST(test_basic_meter_max_value);
    RUN_TEST(test_basic_meter_mid_value);
    RUN_TEST(test_basic_meter_nearly_full_value_still_isnt_full);
    RUN_TEST(test_basic_meter_tiny_value_still_isnt_lit);

    //reversed meter tests    
    RUN_TEST(test_reversed_meter_zero);
    RUN_TEST(test_reversed_meter_max_value);
    RUN_TEST(test_reversed_meter_mid_value);
    RUN_TEST(test_reversed_meter_nearly_full_value_still_isnt_full);
    RUN_TEST(test_reversed_meter_tiny_value_still_isnt_lit);

    //partial meter tests
    RUN_TEST(test_subset_meter_min_value);
    RUN_TEST(test_subset_meter_max_value);
    RUN_TEST(test_subset_meter_mid_value);
  

    UNITY_END();

}
void loop() {
    delay(500);
}