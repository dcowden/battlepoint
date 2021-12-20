#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>

GameState state;
GameSettings settings;
long current_time_ms = 0;
void assertEndedWithWinner ( Team t){
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_ENDED, state.status);
    TEST_ASSERT_EQUAL(t, state.result.winner );
}

void test_in_progress(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 2;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;  
    updateAttackDefendGame(&state,settings,current_time_ms);

    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status);    

}

void test_victory_not_in_overtime(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 10;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateAttackDefendGame(&state,settings,current_time_ms);
    assertEndedWithWinner(Team::BLU);
}

void test_in_progress_within_margin(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 8;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateAttackDefendGame(&state,settings,current_time_ms);
    
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status );    

}

void test_victory_in_overtime(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 12;
    state.time.timeExpired=true;
    state.time.overtimeExpired=false;
    updateAttackDefendGame(&state,settings,current_time_ms);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_fail(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 9;
    state.time.timeExpired=true;
    state.time.overtimeExpired=true;
    updateAttackDefendGame(&state,settings,current_time_ms);
    assertEndedWithWinner(Team::RED);
}


void preTest(){
    settings.gameType = GameType::GAME_TYPE_ATTACK_DEFEND;
}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    preTest();
    
    RUN_TEST(test_in_progress);
    RUN_TEST(test_victory_not_in_overtime);
    RUN_TEST(test_in_progress_within_margin);    
    RUN_TEST(test_victory_in_overtime);
    RUN_TEST(test_end_in_fail);

    UNITY_END();

}
void loop() {
    delay(500);
}