#include <Arduino.h>
#include <Teams.h>
#include <Clock.h>
#include <unity.h>


void test_test(){

};

void test_real_clock(void){    
    RealClock c = RealClock();
    long m = c.milliseconds();
    delay(1000);
    long n = c.milliseconds();
    TEST_ASSERT_EQUAL(1000, n - m );

}

void test_fake_clock(void){
    TestClock tc = TestClock();
    TEST_ASSERT_EQUAL(0, tc.milliseconds());
    tc.setTime(200);
    TEST_ASSERT_EQUAL(200,tc.milliseconds());
    delay(400);
    TEST_ASSERT_EQUAL(200,tc.milliseconds());    

}

void test_fake_clock_add(void){
    TestClock tc = TestClock();
    TEST_ASSERT_EQUAL(0, tc.milliseconds());
    tc.addTime(200);
    TEST_ASSERT_EQUAL(200,tc.milliseconds());
    tc.addTime(400);
    TEST_ASSERT_EQUAL(600,tc.milliseconds());        
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_test);
    RUN_TEST(test_real_clock);
    RUN_TEST(test_fake_clock);
    UNITY_END();
}

void loop() {
    delay(500);
}
