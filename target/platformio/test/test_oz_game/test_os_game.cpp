#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>

GameState state;
GameSettings settings;

void assertEndedWithWinner ( Team t){
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_ENDED, state.status);
    TEST_ASSERT_EQUAL(t, state.result.winner );
}

void test_in_progress_no_capture_yet(){
    state.ownership.owner = Team::NOBODY;
    state.ownership.red_millis=0;
    state.ownership.blu_millis=0;
    state.ownership.capturing=Team::RED;
    state.ownership.capture_hits=3;
    state.timeExpired=false;
    state.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings);
    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_RUNNING);    
}

void test_in_progress_one_team_has_some_time(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=2000;
    state.ownership.blu_millis=0;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    state.timeExpired=false;
    state.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings);
    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_RUNNING);    
}

void test_clear_victory(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    state.timeExpired=false;
    state.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings);
    assertEndedWithWinner( Team::RED);    
}

void test_capture_overtime_time_is_not_up(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=4;
    state.timeExpired=false;
    state.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings);
    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_OVERTIME);  
}

void test_overtime_time_is_up(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=4;
    state.timeExpired=true;
    state.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings);
    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_OVERTIME);  
}

void test_time_is_up_nbody_capturing(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=40000;
    state.ownership.blu_millis=30000;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    state.timeExpired=true;
    state.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings);
    assertEndedWithWinner( Team::RED);   
}

void test_time_is_up_overtime_is_up_too_capturing(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=40000;
    state.ownership.blu_millis=30000;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=0;
    state.timeExpired=true;
    state.overtimeExpired=true;
    updateFirstToOwnTimeGame(&state,settings);
    assertEndedWithWinner( Team::RED);   
}


void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME;
    settings.capture.hits_to_capture=15;
    settings.timed.ownership_time_seconds = 120;
}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    preTest();
    RUN_TEST(test_in_progress_no_capture_yet);
    RUN_TEST(test_clear_victory);
    RUN_TEST(test_capture_overtime_time_is_not_up);
    RUN_TEST(test_overtime_time_is_up);
    RUN_TEST(test_time_is_up_nbody_capturing);
    RUN_TEST(test_time_is_up_overtime_is_up_too_capturing);

    UNITY_END();

}
void loop() {
    delay(500);
}