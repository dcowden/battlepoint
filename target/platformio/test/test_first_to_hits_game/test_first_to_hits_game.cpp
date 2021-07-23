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
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 4;
    state.bluHits.hits = 2;
    state.timeExpired=false;
    state.overtimeExpired=false;    
    updateFirstToHitsGame(&state,settings);

    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status);    

}

void test_victory_not_in_overtime(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 10;
    state.bluHits.hits = 8;
    state.timeExpired=false;
    state.overtimeExpired=false ;   
    updateFirstToHitsGame(&state,settings);
    assertEndedWithWinner(Team::RED);
}

void test_in_progress_within_margin(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 10;
    state.bluHits.hits = 11;
    state.timeExpired=false;
    state.overtimeExpired=false  ;  
    updateFirstToHitsGame(&state,settings);
    
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_OVERTIME, state.status );    

}



void test_victory_in_overtime(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 10;
    state.bluHits.hits = 12;
    state.timeExpired=true;
    state.overtimeExpired=false;
    updateFirstToHitsGame(&state,settings);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_tie(){
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    state.redHits.hits = 11;
    state.bluHits.hits = 11;
    state.timeExpired=true;
    state.overtimeExpired=true;
    updateFirstToHitsGame(&state,settings);
    assertEndedWithWinner(Team::TIE);
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
    RUN_TEST(test_end_in_tie);

    UNITY_END();

}
void loop() {
    delay(500);
}