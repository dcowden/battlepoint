#include <Arduino.h>
#include <button.h>
#include <Clock.h>
#include <unity.h>

TestClock tc = TestClock();
#define BOGUS_TIME 12345

void test_button_just_pressed(void){    

    tc.setTime(BOGUS_TIME);
    ButtonState oldB = { false, 0, 0};

    ButtonState newB = update(1, oldB, 1, &tc );
    TEST_ASSERT_EQUAL(BOGUS_TIME, newB.last_update_millis );
    TEST_ASSERT_TRUE(newB.is_pressed );
    TEST_ASSERT_EQUAL(0, newB.held_millis );
}

void test_button_held(void){
    const long TIME_1 = 1000;
    const long TIME_2 = 3000;
    tc.setTime(TIME_1);
    ButtonState step0 = { false, 0, 0};
    ButtonState step1 = update(1, step0, 1, &tc );
    tc.setTime(TIME_2);
    ButtonState step2 = update(1, step1, 1, &tc );
    TEST_ASSERT_EQUAL(TIME_2, step2.last_update_millis );
    TEST_ASSERT_TRUE(step2.is_pressed );
    TEST_ASSERT_EQUAL(TIME_2 - TIME_1, step2.held_millis  );    
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_button_just_pressed);
    RUN_TEST(test_button_held);
    UNITY_END();
}

void loop() {
    delay(500);
}
