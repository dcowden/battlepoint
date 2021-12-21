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
    state.redHits.hits = 4;
    state.bluHits.hits = 5;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);

    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_PREGAME, state.status);    

}
void test_in_progress_tied(){

    settings.hits.victory_margin = 1;
    state.redHits.hits = 4;
    state.bluHits.hits = 8;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);

    TEST_ASSERT_EQUAL(  GameStatus::GAME_STATUS_PREGAME,state.status);    

}
void test_end_in_time(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 10;
    state.bluHits.hits = 11;
    state.time.timeExpired=true;
    state.time.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);
    
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_OVERTIME,state.status);    

}

void test_victory_in_overtime(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 10;
    state.bluHits.hits = 12;
    state.time.timeExpired=true;
    state.time.overtimeExpired=false;
    updateMostHitsInTimeGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_slight_victory(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 4;
    state.redHits.hits = 11;
    state.bluHits.hits = 12;
    state.time.timeExpired=true;
    state.time.overtimeExpired=true;
    updateMostHitsInTimeGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_tie(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 11;
    state.bluHits.hits = 11;
    state.time.timeExpired=true;
    state.time.overtimeExpired=true;
    updateMostHitsInTimeGame(&state,settings);
    assertEndedWithWinner(Team::TIE);
}


void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_MOST_HITS_IN_TIME;
}

void setup() {
    gamestate_init(&state);
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