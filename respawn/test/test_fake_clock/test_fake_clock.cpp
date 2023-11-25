#include <Clock.h>
#include <unity.h>

void test_fake_clock(void){
    TestClock tc = TestClock();
    TEST_ASSERT_EQUAL(0, tc.milliseconds());
    tc.setTime(200);
    TEST_ASSERT_EQUAL(200,tc.milliseconds());
    TEST_ASSERT_EQUAL(200,tc.milliseconds());    

}

void test_fake_clock_add(void){
    TestClock tc = TestClock();
    TEST_ASSERT_EQUAL(0, tc.milliseconds());
    tc.addMillis(200);
    TEST_ASSERT_EQUAL(200,tc.milliseconds());
    tc.addMillis(400);
    TEST_ASSERT_EQUAL(600,tc.milliseconds());        
}

void test_clock_seconds_since(void){
    TestClock tc = TestClock();
    TEST_ASSERT_EQUAL(0, tc.milliseconds());
    tc.addMillis(4000);
    TEST_ASSERT_EQUAL(3,tc.secondsSince(1000));
}

void RUN_COMMON_TESTS(){
    UNITY_BEGIN();

    RUN_TEST(test_fake_clock);
    RUN_TEST(test_fake_clock_add);
    RUN_TEST(test_clock_seconds_since);
    UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    Serial.begin(115200);
    Serial.setTimeout(500);    
    delay(2000);

    RUN_COMMON_TESTS();
}

void loop() {
    delay(500);
}

#else 
int main(int argc, char **argv) {

    RUN_COMMON_TESTS();
    return 0;
}
#endif