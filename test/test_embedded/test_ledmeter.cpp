#include <Arduino.h>
#include <LedMeter.h>
#include <unity.h>
#include <FastLED.h>

#define LED_COUNT 8
CRGB leds[LED_COUNT];

LedRange testRange [1] = {  { 0, 7 } };
LedMeter simpleMeter = LedMeter(leds,testRange,1);

LedRange reversedTestRange [1] = {  { 7, 0 } };
LedMeter reversedMeter = LedMeter(leds,reversedTestRange,1);

LedRange doubleTestRange [2] = {  {0, 3}, { 4, 7}  };
LedMeter twoRangeMeter = LedMeter(leds,doubleTestRange,2);

LedRange mirroredTestRange [2] = {  {0, 3}, { 7, 4}  };
LedMeter mirroredRangeMeter = LedMeter(leds,mirroredTestRange,2);

CRGB ALL_BLUE[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                            CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue };

CRGB ALL_BLACK[LED_COUNT] = {CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black,
                            CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black };

void setupMeter(LedMeter* meter){    
    meter->setMaxValue(100);
    meter->setBgColor(CRGB::Black);
    meter->setFgColor(CRGB::Blue);
    for ( int i=0;i<LED_COUNT;i++){
        leds[i] = CRGB::Black;
    }
}

void assert_leds_equal(CRGB* expected){
    for (int i=0;i<LED_COUNT;i++,expected++){
        TEST_ASSERT_EQUAL(*expected,leds[i]);
    }
}

void test_meter_initially_all_black(void) {
    assert_leds_equal(ALL_BLACK);
}

void test_basic_meter_bounds(void){
    TEST_ASSERT_EQUAL(100, simpleMeter.getMaxValue());    
}

void test_basic_meter_zero(void){
    simpleMeter.setValue(0);
    assert_leds_equal(ALL_BLACK);    
}
void test_basic_meter_max_value(void){
    simpleMeter.setValue(100);
    assert_leds_equal(ALL_BLUE);
}

void test_basic_meter_mid_value(void){
    simpleMeter.setValue(50);
    CRGB expected[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue,CRGB::Blue,
                                CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black };
    assert_leds_equal(expected);
}

void test_basic_meter_nearly_full_value_still_isnt_full(void){
    simpleMeter.setValue(95);
    CRGB expected[LED_COUNT] = {CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                                CRGB::Blue,CRGB::Blue,CRGB::Blue, CRGB::Black };
    assert_leds_equal(expected);
}

void test_basic_meter_tiny_value_still_isnt_lit(void){
    simpleMeter.setValue(11);
    assert_leds_equal(ALL_BLACK);
}

void test_reversed_meter_zero(void){
    reversedMeter.setValue(0);
    assert_leds_equal(ALL_BLACK);    
}
void test_reversed_meter_max_value(void){
    reversedMeter.setValue(100);
    assert_leds_equal(ALL_BLUE);
}

void test_reversed_meter_mid_value(void){
    reversedMeter.setValue(50);
    CRGB expected[LED_COUNT] = { CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black, 
                                CRGB::Blue,CRGB::Blue,CRGB::Blue,CRGB::Blue };
    assert_leds_equal(expected);
}

void test_reversed_meter_nearly_full_value_still_isnt_full(void){
    reversedMeter.setValue(95);
    CRGB expected[LED_COUNT] = {CRGB::Black, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                                CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue };
    assert_leds_equal(expected);
}

void test_reversed_meter_tiny_value_still_isnt_lit(void){
    reversedMeter.setValue(11);
    assert_leds_equal(ALL_BLACK);
}

void test_double_meter_zero(void){
    twoRangeMeter.setValue(0);
    assert_leds_equal(ALL_BLACK);    
}

void test_double_meter_max_value(void){
    twoRangeMeter.setValue(100);
    assert_leds_equal(ALL_BLUE);
}

void test_double_meter_mid_value(void){
    twoRangeMeter.setValue(50);
    CRGB expected[LED_COUNT] = { CRGB::Blue,CRGB::Blue, CRGB::Black, CRGB::Black,
                                 CRGB::Blue,CRGB::Blue, CRGB::Black, CRGB::Black };
    assert_leds_equal(expected);
}

void test_double_meter_nearly_full_value_still_isnt_full(void){
    twoRangeMeter.setValue(95);
    CRGB expected[LED_COUNT] = {CRGB::Blue,CRGB::Blue, CRGB::Blue,CRGB::Black, 
                                CRGB::Blue,CRGB::Blue, CRGB::Blue,CRGB::Black };
    assert_leds_equal(expected);
}

void test_double_meter_tiny_value_still_isnt_lit(void){
    twoRangeMeter.setValue(18);
    assert_leds_equal(ALL_BLACK);
}

void test_mirrored_meter_zero(void){
    twoRangeMeter.setValue(0);
    assert_leds_equal(ALL_BLACK);    
}

void test_mirrored_meter_max_value(void){
    twoRangeMeter.setValue(100);
    assert_leds_equal(ALL_BLUE);
}

void test_mirrored_meter_mid_value(void){
    twoRangeMeter.setValue(50);
    CRGB expected[LED_COUNT] = { CRGB::Blue,CRGB::Blue, CRGB::Black, CRGB::Black,
                                 CRGB::Black,CRGB::Black, CRGB::Blue, CRGB::Blue };
    assert_leds_equal(expected);
}

void test_mirrored_meter_nearly_full_value_still_isnt_full(void){
    twoRangeMeter.setValue(95);
    CRGB expected[LED_COUNT] = {CRGB::Blue,CRGB::Blue, CRGB::Blue,CRGB::Black, 
                                CRGB::Black,CRGB::Blue, CRGB::Blue,CRGB::Blue };
    assert_leds_equal(expected);
}

void test_mirrored_meter_tiny_value_still_isnt_lit(void){
    twoRangeMeter.setValue(18);
    assert_leds_equal(ALL_BLACK);
}

void setup() {
    
    delay(2000);
    Serial.begin(115200);
    UNITY_BEGIN();

    //simple meter tests
    setupMeter(&simpleMeter);
    RUN_TEST(test_meter_initially_all_black);
    RUN_TEST(test_basic_meter_bounds);
    RUN_TEST(test_basic_meter_zero);
    RUN_TEST(test_basic_meter_max_value);
    RUN_TEST(test_basic_meter_mid_value);
    RUN_TEST(test_basic_meter_nearly_full_value_still_isnt_full);
    RUN_TEST(test_basic_meter_tiny_value_still_isnt_lit);

    //reversed meter tests
    setupMeter(&reversedMeter);
    RUN_TEST(test_reversed_meter_zero);
    RUN_TEST(test_reversed_meter_max_value);
    RUN_TEST(test_reversed_meter_mid_value);
    RUN_TEST(test_reversed_meter_nearly_full_value_still_isnt_full);
    RUN_TEST(test_reversed_meter_tiny_value_still_isnt_lit);

    //tworange meter
    setupMeter(&twoRangeMeter);
    RUN_TEST(test_double_meter_zero);
    RUN_TEST(test_double_meter_max_value);
    RUN_TEST(test_double_meter_mid_value);
    RUN_TEST(test_double_meter_nearly_full_value_still_isnt_full);
    RUN_TEST(test_double_meter_tiny_value_still_isnt_lit);

    //two range, mirrored meter
    setupMeter(&mirroredRangeMeter);
    RUN_TEST(test_mirrored_meter_zero);
    RUN_TEST(test_mirrored_meter_max_value);
    RUN_TEST(test_mirrored_meter_mid_value);
    RUN_TEST(test_mirrored_meter_nearly_full_value_still_isnt_full);
    RUN_TEST(test_mirrored_meter_tiny_value_still_isnt_lit);

    UNITY_END();

}
void loop() {
    delay(500);
}