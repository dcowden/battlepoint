#include <Arduino.h>
#include <LedController.h>
#include <unity.h>
#include <FastLED.h>

#define LED_COUNT 2

CRGB testLeds[LED_COUNT];
CRGB BLUE = CRGB::Blue;
CRGB BLACK = CRGB::Black;
CRGB ALL_BLUE[LED_COUNT] = {BLUE, BLUE};
CRGB ALL_BLACK[LED_COUNT] = {BLACK, BLACK };
CRGB GREEN = CRGB::Green;
CRGB RED = CRGB::Red;

void resetLEDS(){    
    for ( int i=0;i<LED_COUNT;i++){
        testLeds[i] = CRGB::Black;
    }
}

LedPixel ledPixels[] = {
    LedPixel (testLeds,0,CRGB::Black,CRGB::Black),
    LedPixel (testLeds,1,CRGB::Black,CRGB::Black)
};
LedGroup ledGroup(ledPixels,2);

//LedPixel pixel1(testLeds,0,CRGB::Black,CRGB::Black);
//LedPixel pixel2(testLeds,1,CRGB::Black,CRGB::Black);


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

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    
    //RUN_TEST(test_initially_all_black);
    RUN_TEST(test_setting_solid_color);

    UNITY_END();

}
void loop() {
    delay(500);
}