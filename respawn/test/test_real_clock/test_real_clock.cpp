#include <Clock.h>
#include <unity.h>
#include <Arduino.h>

void test_real_clock(void){  

    RealClock c = RealClock();
    long m = c.milliseconds();
    delay(10);
    long n = c.milliseconds();
    TEST_ASSERT_EQUAL(1000, n - m );

}

void RUN_TESTS(){
    UNITY_BEGIN();
    RUN_TEST(test_real_clock);
    UNITY_END();
}


void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    void RUN_TESTS();

}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}
