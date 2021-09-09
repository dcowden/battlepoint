#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>

#define BOGUS_INITIAL_TIME 1000
#define BOGUS_UPDATED_TIME 2000
GameSettings settings;
GameState currentState;

void test_red_hit(){
    SensorState sensorState;
    sensorState.rightScan.was_hit=false;
    sensorState.leftScan.was_hit=true;    
    currentState.redHits.hits=1;
    currentState.redHits.last_hit_millis=BOGUS_INITIAL_TIME;
    updateGameHits(&currentState,&sensorState,BOGUS_UPDATED_TIME);

    TEST_ASSERT_EQUAL_INT(BOGUS_UPDATED_TIME,currentState.redHits.last_hit_millis);
    TEST_ASSERT_EQUAL_INT(2,currentState.redHits.hits);
}

void test_both_hit(){
    SensorState sensorState;
    sensorState.rightScan.was_hit=true;
    sensorState.leftScan.was_hit=true;    
    currentState.redHits.hits=1;
    currentState.bluHits.hits=10;
    currentState.redHits.last_hit_millis=BOGUS_INITIAL_TIME;
    currentState.bluHits.last_hit_millis=BOGUS_INITIAL_TIME;
    updateGameHits(&currentState,&sensorState,BOGUS_UPDATED_TIME);

    TEST_ASSERT_EQUAL_INT(BOGUS_UPDATED_TIME,currentState.redHits.last_hit_millis);
    TEST_ASSERT_EQUAL_INT(2,currentState.redHits.hits);
    TEST_ASSERT_EQUAL_INT(BOGUS_UPDATED_TIME,currentState.bluHits.last_hit_millis);
    TEST_ASSERT_EQUAL_INT(11,currentState.bluHits.hits);    
}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    RUN_TEST(test_red_hit);
    RUN_TEST(test_both_hit);

    UNITY_END();

}
void loop() {
    delay(500);
}