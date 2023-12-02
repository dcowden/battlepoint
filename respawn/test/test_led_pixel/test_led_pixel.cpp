#include <Arduino.h>
#include <LedController.h>
#include <unity.h>
#include <FastLED.h>
#include <Clock.h>

#define LED_COUNT 2

TestClock clock;

CRGB testLeds[LED_COUNT];

CRGB ALL_BLUE[LED_COUNT] = {CRGB::Blue, CRGB::Blue};
CRGB GREEN = CRGB::Green;
CRGB RED = CRGB::Red;
CRGB BLACK = CRGB::Black;
CRGB BLUE = CRGB::Blue;
CRGB ALL_BLACK[LED_COUNT] = {CRGB::Black, CRGB::Black };

void resetLEDS(){    
    for ( int i=0;i<LED_COUNT;i++){
        testLeds[i] = CRGB::Black;
    }
}

LedPixel pixel1(&clock,testLeds,0,CRGB::Black,CRGB::Black);
LedPixel pixel2(&clock,testLeds,1,CRGB::Black,CRGB::Black);

void assert_led_equals(CRGB* expected, int i){
    TEST_ASSERT_EQUAL(expected->blue, testLeds[i].blue);
    TEST_ASSERT_EQUAL(expected->red, testLeds[i].red);
    TEST_ASSERT_EQUAL(expected->green, testLeds[i].green);
}
void assert_leds_equal(CRGB* expected){
    for (int i=0;i<LED_COUNT;i++,expected++){
        assert_led_equals(expected,i);
    }
}

void test_initially_all_black(void) {
    resetLEDS();
    assert_leds_equal(ALL_BLACK);
}

void test_blinking_red_green(void){
    resetLEDS();
    clock.setTime(0);
    pixel1.setBlinkingColor(CRGB::Red, CRGB::Green,1000);
    pixel1.update();
    assert_led_equals(&GREEN,0);

 
    clock.addMillis(1001);
    pixel1.update();
    assert_led_equals(&RED,0);

    clock.addMillis(1001);
    pixel1.update();
    assert_led_equals(&GREEN,0);

    clock.addMillis(1001);
    pixel1.update();
    assert_led_equals(&RED,0);    

}

void test_solid_color(void) {
    resetLEDS();
    clock.setTime(0);
    pixel1.setSolidColor(CRGB::Blue);
    pixel1.update();
    pixel1.update();
    assert_led_equals(&BLUE,0);

    pixel2.setSolidColor(CRGB::Blue);    
    pixel1.update();
    pixel1.update();
    assert_leds_equal(ALL_BLUE);
}

void test_blinking_solid_blinking(void){
    resetLEDS();
    clock.setTime(0);
    pixel1.setBlinkingColor(CRGB::Red, CRGB::Green,1000);
    pixel1.update();
    assert_led_equals(&GREEN,0);

    clock.addMillis(1100);
    pixel1.update();
    assert_led_equals(&RED,0);

    clock.addMillis(1100);
    pixel1.update();
    assert_led_equals(&GREEN,0);


    pixel1.setSolidColor(CRGB::Blue);
    pixel1.update();
    assert_led_equals(&BLUE,0);

    clock.addMillis(1100);
    pixel1.update();
    assert_led_equals(&BLUE,0);

    //now set blinking again
    pixel1.setBlinkingColor(CRGB::Red, CRGB::Green,1000);
    pixel1.update();
    assert_led_equals(&GREEN,0);

    clock.addMillis(1100);
    pixel1.update();
    assert_led_equals(&RED,0);

    clock.addMillis(1100);
    pixel1.update();
    assert_led_equals(&GREEN,0); 
}

//TODO: add tests for updateController
void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    
    RUN_TEST(test_initially_all_black);
    RUN_TEST(test_solid_color);
    RUN_TEST(test_blinking_red_green);
    RUN_TEST(test_blinking_solid_blinking);

    UNITY_END();

}
void loop() {
    delay(500);
}