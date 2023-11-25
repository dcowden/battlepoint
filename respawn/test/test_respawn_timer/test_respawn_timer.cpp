#include <Arduino.h>
#include <RespawnTimer.h>
#include <ArduinoLog.h>
#include <unity.h>

RespawnTimer simpleTimer {
    .id=0,
    .durationMillis=0,
    .startMillis=MILLIS_NOT_STARTED,
    .endMillis=0,
    .respawnImminentMillis=0,
    .resetMillis=0
};

void test_init_starts_disabled(void){
    disableTimer(&simpleTimer);
    TEST_ASSERT_EQUAL( isAvailable(&simpleTimer,millis()), false);
    TEST_ASSERT_EQUAL(computeTimerState(&simpleTimer, millis() ), RespawnTimerState::IDLE);
}

void test_test_timer_stages(void){
    long currentTimeMillis = 0;
    long respawnDurationMillis = 20000;
    startTimer(&simpleTimer,respawnDurationMillis,currentTimeMillis);
    TEST_ASSERT_EQUAL(computeTimerState(&simpleTimer, currentTimeMillis ), RespawnTimerState::RESPAWNING);
    TEST_ASSERT_EQUAL(computeTimerState(&simpleTimer, 10000 ), RespawnTimerState::RESPAWNING);
    TEST_ASSERT_EQUAL(computeTimerState(&simpleTimer, 18000 ), RespawnTimerState::IMMINENT);
    TEST_ASSERT_EQUAL(computeTimerState(&simpleTimer, respawnDurationMillis + DEFAULT_READY_TIME_LEFT_MILLIS - 1000 ), RespawnTimerState::FINISHED);
    TEST_ASSERT_EQUAL(computeTimerState(&simpleTimer, respawnDurationMillis + DEFAULT_GO_SIGNAL_MILLIS + 1000 ), RespawnTimerState::IDLE);
}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();

    
    RUN_TEST(test_init_starts_disabled);


    UNITY_END();

}
void loop() {
    delay(500);
}