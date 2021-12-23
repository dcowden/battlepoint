#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>

GameState state;
GameSettings settings;
long current_time_ms = 0;

void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_HITS;
    settings.timed.max_duration_seconds=20;
    settings.timed.countdown_start_seconds=5;
    settings.hits.to_win = 10;
    settings.hits.victory_margin = 2;
    settings.capture.hits_to_capture=10;
    settings.timed.countdown_start_seconds=2;
    startGame(&state,&settings,current_time_ms);
}

void assertEndedWithWinner ( Team t){
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_ENDED, state.status);
    TEST_ASSERT_EQUAL(t, state.result.winner );
}

void test_in_progress(){
    preTest();
    state.ownership.capture_hits = 9;
    state.redHits.hits = 4;
    state.bluHits.hits = 2;
    updateGame(&state,settings,6000);    
    updateGame(&state,settings,6100);
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status);    
}

void test_victory_not_in_overtime(){
    preTest();    
    state.redHits.hits = 10;
    state.bluHits.hits = 8; 
    updateGame(&state,settings,6000);
    updateGame(&state,settings,6100);
    assertEndedWithWinner(Team::RED);
}

void test_in_progress_within_margin(){
    preTest();
    state.redHits.hits = 10;
    state.bluHits.hits = 11;
    updateGame(&state,settings,6000);
    updateGame(&state,settings,6100);
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_OVERTIME, state.status );    

}

void test_victory_in_overtime(){
    preTest();
    state.redHits.hits = 10;
    state.bluHits.hits = 12;
    updateGame(&state,settings,6000);
    updateGame(&state,settings,6100);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_tie(){
    preTest();
    state.redHits.hits = 11;
    state.bluHits.hits = 11;
    updateGame(&state,settings,6000);
    updateGame(&state,settings,61000);
    assertEndedWithWinner(Team::TIE);
}


void setup() {

    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();

    
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