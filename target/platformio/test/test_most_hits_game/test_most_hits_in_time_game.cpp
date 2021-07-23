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

    settings.hits.victory_margin = 2;
    state.hits.red_hits = 4;
    state.hits.blu_hits = 5;
    state.timeExpired=false;
    state.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);

    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_RUNNING);    

}
void test_in_progress_tied(){

    settings.hits.victory_margin = 1;
    state.hits.red_hits = 4;
    state.hits.blu_hits = 8;
    state.timeExpired=false;
    state.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);

    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_RUNNING);    

}
void test_end_in_time(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.hits.red_hits = 10;
    state.hits.blu_hits = 11;
    state.timeExpired=true;
    state.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);
    
    TEST_ASSERT_EQUAL( state.status, GameStatus::GAME_STATUS_OVERTIME);    

}

void test_victory_in_overtime(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.hits.red_hits = 10;
    state.hits.blu_hits = 12;
    state.timeExpired=true;
    state.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_slight_victory(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 4;
    state.hits.red_hits = 11;
    state.hits.blu_hits = 12;
    state.timeExpired=true;
    state.overtimeExpired=true;
    updateMostHitsInTimeGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_tie(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.hits.red_hits = 11;
    state.hits.blu_hits = 11;
    state.timeExpired=true;
    state.overtimeExpired=true;
    updateMostHitsInTimeGame(&state,settings);
    assertEndedWithWinner(Team::TIE);
}


void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME;
}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    preTest();
    RUN_TEST(test_in_progress);
    RUN_TEST(test_in_progress_tied);
    RUN_TEST(test_end_in_time);
    RUN_TEST(test_victory_in_overtime);
    RUN_TEST(test_end_in_slight_victory);
    RUN_TEST(test_end_in_tie);

    UNITY_END();

}
void loop() {
    delay(500);
}