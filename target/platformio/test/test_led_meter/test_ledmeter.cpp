#include <Arduino.h>
#include <LedMeter.h>
#include <unity.h>
#include <FastLED.h>
//#define BP_DEBUG 1
#define LED_COUNT 8
CRGB leds[LED_COUNT];

CRGB ALL_BLUE[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                            CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue };

CRGB ALL_BLACK[LED_COUNT] = {CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black,
                            CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black };

void resetLEDS(){    
    for ( int i=0;i<LED_COUNT;i++){
        leds[i] = CRGB::Black;
    }
}

LedMeter simpleMeter { 
    .startIndex=0, 
    .endIndex=7, 
    .max_val=100, 
    .val=0, 
    .flash_interval_millis=FlashInterval::FLASH_NONE, 
    .last_flash_millis=0,
    .fgColor=CRGB::Blue, 
    .bgColor=CRGB::Black };  //8 lights

LedMeter reversedMeter { 
    .startIndex=7, 
    .endIndex=0, 
    .max_val=100,
    .val=0,
    .flash_interval_millis=FlashInterval::FLASH_NONE, 
    .last_flash_millis=0,     
    .fgColor=CRGB::Blue, 
    .bgColor=CRGB::Black }; //8 lights

LedMeter subsetMeter { 
    .startIndex=0, 
    .endIndex=3, 
    .max_val=100,
    .val=0, 
    .flash_interval_millis=FlashInterval::FLASH_NONE, 
    .last_flash_millis=0,         
    .fgColor=CRGB::Blue, 
    .bgColor=CRGB::Black }; //4 lights, first half

void assert_leds_equal(CRGB* expected, int debug){
    CRGB* debug_ptr = expected;
    #ifdef BP_DEBUG
        for (int i=0;i<LED_COUNT;i++,debug_ptr++){        
            Serial.print("[expected=");
            Serial.print(*debug_ptr);
            Serial.print(",actual=");
            Serial.print(leds[i]);
            Serial.println("]");
        }
    }
    #endif
    for (int i=0;i<LED_COUNT;i++,expected++){
        TEST_ASSERT_EQUAL(*expected,leds[i]);
    }
}

void meterAssertVal(LedMeter meter, int val, CRGB* expectedVals){
    resetLEDS();
    meter.val = val;
    updateLedMeter(leds,meter);
    assert_leds_equal(expectedVals,0);
}

void test_meter_initially_all_black(void) {
    resetLEDS();
    assert_leds_equal(ALL_BLACK,0);
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

void setup() {
    
    delay(1000);
    Serial.begin(115200);
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