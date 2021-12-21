#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include<Teams.h>
#include <ArduinoLog.h>
#define BOGUS_UPDATE_TIME 2000
#define INITIAL_TIME 400
#define HITS_TO_CAPTURE 10
#define OVERTIME_CAPTURE_MILLIS 30
GameSettings settings;
GameState currentState;

void ASSERT_OWNER_AND_CAPTURING(Ownership o, Team owner, Team capturing){
    TEST_ASSERT_EQUAL(capturing, o.capturing);
    TEST_ASSERT_EQUAL(owner, o.owner);
}
void ASSERT_RED_AND_BLUE_OWNERSHIP_TIME(Ownership o, long red_time_expected, long blue_time_expected){
    TEST_ASSERT_EQUAL(blue_time_expected,o.blu_millis);
    TEST_ASSERT_EQUAL(red_time_expected, o.red_millis);
}

void preTest(){
    settings.capture.hits_to_capture=HITS_TO_CAPTURE;
    currentState.time.last_update_millis=INITIAL_TIME;
    settings.capture.capture_overtime_seconds=OVERTIME_CAPTURE_MILLIS;
    settings.capture.capture_decay_rate_secs_per_hit=0;
}


void test_owner_no_capture_yet_no_owner(){
    preTest();    
    const int INITIAL_HITS=8;
    currentState.ownership.capturing=Team::RED;
    currentState.ownership.owner=Team::NOBODY;
    currentState.ownership.capture_hits=INITIAL_HITS;    

    updateOwnership(&currentState,settings,BOGUS_UPDATE_TIME);
    
    TEST_ASSERT_EQUAL(INITIAL_HITS,currentState.ownership.capture_hits);
    TEST_ASSERT_EQUAL(0, currentState.ownership.overtime_remaining_millis);
    ASSERT_OWNER_AND_CAPTURING(currentState.ownership,Team::NOBODY,Team::RED);
    ASSERT_RED_AND_BLUE_OWNERSHIP_TIME(currentState.ownership,0,0);
    
}

void test_capture_with_current_owner(){
    preTest();
    currentState.ownership.capturing=Team::RED;
    currentState.ownership.owner=Team::BLU;
    currentState.ownership.capture_hits=HITS_TO_CAPTURE;
    ASSERT_OWNER_AND_CAPTURING(currentState.ownership,Team::BLU,Team::RED);
    updateOwnership(&currentState,settings,BOGUS_UPDATE_TIME);

    TEST_ASSERT_EQUAL(0,currentState.ownership.capture_hits);
    TEST_ASSERT_EQUAL(OVERTIME_CAPTURE_MILLIS, currentState.ownership.overtime_remaining_millis);
    ASSERT_OWNER_AND_CAPTURING(currentState.ownership,Team::RED,Team::BLU);
    ASSERT_RED_AND_BLUE_OWNERSHIP_TIME(currentState.ownership,0,BOGUS_UPDATE_TIME-INITIAL_TIME);

}



void setup() {
    gamestate_init(&currentState);
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);

    UNITY_BEGIN();
    RUN_TEST(test_owner_no_capture_yet_no_owner);
    RUN_TEST(test_capture_with_current_owner);

    UNITY_END();

}
void loop() {
    delay(500);
}