#include <RespawnTimer.h>
#include <unity.h>

#ifdef ARDUINO
   #include<Arduino.h>
#endif

#ifdef UNIT_TEST
    #include <ArduinoFake.h>
#endif

RespawnTimer simpleTimer = RespawnTimer();


void test_init_starts_disabled(void){
    TEST_ASSERT_TRUE( simpleTimer.isIdle(0));
    TEST_ASSERT_EQUAL(simpleTimer.computeTimerState(0), RespawnTimerState::IDLE);
}
void test_disabled_timer_not_available(void){
    simpleTimer.disable();
    TEST_ASSERT_TRUE( simpleTimer.isIdle(0));
}

void RUN_UNITY_TESTS() {
    UNITY_BEGIN();
    RUN_TEST(test_init_starts_disabled);
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

    RUN_UNITY_TESTS();
}

void loop() {
    delay(500);
}

#else
#include <ArduinoFake.h>

int main(int argc, char **argv) {
    RUN_UNITY_TESTS();
    return 0;
}

#endif