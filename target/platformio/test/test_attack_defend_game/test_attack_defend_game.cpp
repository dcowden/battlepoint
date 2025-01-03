#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>

GameState state;
GameSettings settings;

long current_time_ms = 0;


void preTest(){
    settings.gameType = GameType::GAME_TYPE_ATTACK_DEFEND;
}

void assertEndedWithWinner ( Team t){
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_ENDED, state.status);
    TEST_ASSERT_EQUAL(t, state.result.winner );
}

void test_in_progress(){
    preTest();
    
    settings.timed.countdown_start_seconds=5;
    settings.timed.max_duration_seconds=20;
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 2;
    startGame(&state,&settings,current_time_ms);

    updateAttackDefendGame(&state,settings,2000);

    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_PREGAME, state.status);    

}

void test_victory_not_in_overtime(){
    preTest();
    settings.timed.countdown_start_seconds=2;
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    settings.capture.capture_decay_rate_secs_per_hit=600; //disabled effetively

    startGame(&state,&settings,current_time_ms);
    state.ownership.capture_hits = 10;
    updateGame(&state,settings,6000);
    updateGame(&state,settings,6200);
    assertEndedWithWinner(Team::BLU);
}

void test_in_progress_within_margin(){
    preTest();
    settings.timed.max_duration_seconds=20;
    settings.timed.countdown_start_seconds=5;
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    state.ownership.capture_hits = 8;
    startGame(&state,&settings,current_time_ms);
    updateGame(&state,settings,6000);
    updateGame(&state,settings,6200);
    
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_RUNNING, state.status );    

}

void test_victory_in_overtime(){
    preTest();
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    
    settings.capture.capture_decay_rate_secs_per_hit=600; //disabled effetively

    startGame(&state,&settings,current_time_ms);
    state.ownership.capture_hits = 12;   
    updateGame(&state,settings,6000);
    updateGame(&state,settings,6200);
    assertEndedWithWinner(Team::BLU);
}
void test_end_in_fail(){
    preTest();
    settings.capture.hits_to_capture=10;
    settings.hits.victory_margin = 2;
    settings.timed.max_duration_seconds=20;
    settings.timed.countdown_start_seconds=5;    
    startGame(&state,&settings,current_time_ms);
    state.ownership.capture_hits = 7;
    updateGame(&state,settings,25000);    
    updateGame(&state,settings,25100);    


    assertEndedWithWinner(Team::RED);
}



void setup() {

    delay(500);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    
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