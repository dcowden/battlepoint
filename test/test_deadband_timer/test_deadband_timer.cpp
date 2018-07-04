#include <Arduino.h>
#include <Teams.h>
#include <CooldownTimer.h>
#include <unity.h>
#include <Clock.h>


void test_basic_timer(void){
    TestClock clock = TestClock();
    CooldownTimer ct = CooldownTimer(500, &clock);
    TEST_ASSERT_TRUE ( ct.canRun() ) ;
    TEST_ASSERT_FALSE ( ct.canRun() );
    clock.addTime(100);
    TEST_ASSERT_FALSE ( ct.canRun() );
    clock.addTime(600);
    TEST_ASSERT_TRUE ( ct.canRun()) ;
    clock.addTime(600);
    TEST_ASSERT_TRUE ( ct.canRun()) ;    
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_basic_timer);
    UNITY_END();
}

void loop() {
    delay(500);
}
