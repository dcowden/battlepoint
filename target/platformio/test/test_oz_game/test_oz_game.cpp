#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>
#define DEFAULT_UPDATE_TIME 10000

GameState state;
GameSettings settings;
long current_time_ms = 0;

void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME;
    settings.timed.max_duration_seconds=20;
    settings.timed.countdown_start_seconds=5;
    settings.timed.max_overtime_seconds=10;
    settings.capture.hits_to_capture=15;
    settings.timed.ownership_time_seconds = 120;
    settings.timed.countdown_start_seconds=2;
    startGame(&state,&settings,current_time_ms);
}

void assertEndedWithWinner ( Team t){
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_ENDED, state.status);
    TEST_ASSERT_EQUAL(t, state.result.winner );
}

void test_in_progress_no_capture_yet(){
    preTest();
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_PREGAME, state.status );    
}

void test_in_progress_one_team_has_some_time(){
    preTest();    
    state.ownership.owner = Team::RED;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_PREGAME,state.status);    
}

void test_clear_victory(){
    preTest();  
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;

    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    assertEndedWithWinner( Team::RED);    
}

void test_capture_overtime_time_is_not_up(){
    preTest();
    state.ownership.overtime_remaining_millis = 200;
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=4;

    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_OVERTIME, state.status );  
}

void test_overtime_time_is_up(){
    preTest();
    state.ownership.overtime_remaining_millis = 200;
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=4;

    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_OVERTIME,state.status );  
}

void test_time_is_up_nbody_capturing(){
    preTest();
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=40000;
    state.ownership.blu_millis=30000;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    updateGame(&state,settings,26000); //game ends at t=25
    updateGame(&state,settings,27000);
    assertEndedWithWinner( Team::RED);   
}

void test_time_is_up_overtime_is_up_too_capturing(){
    preTest();
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=40000;
    state.ownership.blu_millis=30000;
    state.ownership.capturing=Team::BLU;
    updateGame(&state,settings,60000);
    updateGame(&state,settings,61000);
    assertEndedWithWinner( Team::RED);   
}


void setup() {

    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    
    //RUN_TEST(test_time_is_up_nbody_capturing);
    RUN_TEST(test_in_progress_no_capture_yet);
    RUN_TEST(test_in_progress_one_team_has_some_time);
    RUN_TEST(test_clear_victory);
    RUN_TEST(test_capture_overtime_time_is_not_up);
    RUN_TEST(test_overtime_time_is_up);
    RUN_TEST(test_time_is_up_nbody_capturing);
    RUN_TEST(test_time_is_up_overtime_is_up_too_capturing);
    /** **/
    UNITY_END();

}
void loop() {
    delay(500);
}