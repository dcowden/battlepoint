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

void test_in_progress(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.hits.blu_hits = 2;
    state.timeExpired=false;
    state.overtimeExpired=false;    
    updateAttackDefendGame(&state,settings);

    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status);    

}

void test_victory_not_in_overtime(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.hits.blu_hits = 10;
    state.timeExpired=false;
    state.overtimeExpired=false ;   
    updateAttackDefendGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}

void test_in_progress_within_margin(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.hits.blu_hits = 8;
    state.timeExpired=false;
    state.overtimeExpired=false  ;  
    updateAttackDefendGame(&state,settings);
    
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status );    

}

void test_victory_in_overtime(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.hits.blu_hits = 12;
    state.timeExpired=true;
    state.overtimeExpired=false;
    updateAttackDefendGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_fail(){
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.hits.blu_hits = 9;
    state.timeExpired=true;
    state.overtimeExpired=true;
    updateAttackDefendGame(&state,settings);
    assertEndedWithWinner(Team::RED);
}


void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_HITS;
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