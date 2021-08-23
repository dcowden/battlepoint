#include <Arduino.h>
#include <trigger.h>
#include <unity.h>


TestClock tc = TestClock();

void test_trigger_runs_once_only(void){
    Trigger t = Trigger(&tc, 20);
    tc.setTime(1000);
    t.trigger();
    tc.setTime(1100);
    t.trigger();
    TEST_ASSERT_TRUE(t.isTriggered() );
    TEST_ASSERT_FALSE(t.isTriggered() );
    tc.setTime(1119);
    TEST_ASSERT_FALSE(t.isTriggered() );
    tc.setTime(1200);
    TEST_ASSERT_FALSE(t.isTriggered() );
    t.trigger();
    tc.setTime(1300);
    TEST_ASSERT_TRUE(t.isTriggered() );
    TEST_ASSERT_FALSE(t.isTriggered() );
}


void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_trigger_runs_once_only);
    UNITY_END();
}

void loop() {
    delay(500);
}