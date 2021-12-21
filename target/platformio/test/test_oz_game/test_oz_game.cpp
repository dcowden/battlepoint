#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>
#define DEFAULT_UPDATE_TIME 1000

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
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_PREGAME, state.status );    
}

void test_in_progress_one_team_has_some_time(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=2000;
    state.ownership.blu_millis=0;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_PREGAME,state.status);    
}

void test_clear_victory(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    assertEndedWithWinner( Team::RED);    
}

void test_capture_overtime_time_is_not_up(){
    state.ownership.overtime_remaining_millis = 200;
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=4;
    state.time.timeExpired=false;
    state.time.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_OVERTIME, state.status );  
}

void test_overtime_time_is_up(){
    state.ownership.overtime_remaining_millis = 200;
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=120001;
    state.ownership.blu_millis=32220;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=4;
    state.time.timeExpired=true;
    state.time.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    TEST_ASSERT_EQUAL( GameStatus::GAME_STATUS_OVERTIME,state.status );  
}

void test_time_is_up_nbody_capturing(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=40000;
    state.ownership.blu_millis=30000;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.capture_hits=0;
    state.time.timeExpired=true;
    state.time.overtimeExpired=false;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    assertEndedWithWinner( Team::RED);   
}

void test_time_is_up_overtime_is_up_too_capturing(){
    state.ownership.owner = Team::RED;
    state.ownership.red_millis=40000;
    state.ownership.blu_millis=30000;
    state.ownership.capturing=Team::BLU;
    state.ownership.capture_hits=0;
    state.time.timeExpired=true;
    state.time.overtimeExpired=true;
    updateFirstToOwnTimeGame(&state,settings,DEFAULT_UPDATE_TIME);
    assertEndedWithWinner( Team::RED);   
}


void preTest(){
    settings.gameType = GameType::GAME_TYPE_KOTH_FIRST_TO_OWN_TIME;
    settings.capture.hits_to_capture=15;
    settings.timed.ownership_time_seconds = 120;
    
    state.ownership.blu_millis=0;
    state.ownership.red_millis=0;
    state.ownership.capture_hits=0;
    state.ownership.capturing=Team::NOBODY;
    state.ownership.last_decay_millis=0;
    state.ownership.last_hit_millis=0;
    state.ownership.overtime_remaining_millis = 0;
    state.ownership.owner=Team::NOBODY;
}

void setup() {
    gamestate_init(&state);
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    

    //preTest(); 
    //RUN_TEST(test_time_is_up_nbody_capturing);

    
    preTest();
    RUN_TEST(test_in_progress_no_capture_yet);
    preTest();
    RUN_TEST(test_in_progress_one_team_has_some_time);
    preTest();
    RUN_TEST(test_clear_victory);
    preTest(); 
    RUN_TEST(test_capture_overtime_time_is_not_up);
    preTest();
    RUN_TEST(test_overtime_time_is_up);
    preTest();
    RUN_TEST(test_time_is_up_nbody_capturing);
    preTest();
    RUN_TEST(test_time_is_up_overtime_is_up_too_capturing);
    /** **/
    UNITY_END();

}
void loop() {
    delay(500);
}