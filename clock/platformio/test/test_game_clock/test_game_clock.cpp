#include <Arduino.h>
#include <GameClock.h>
#include <unity.h>

GameClockState cs;

long START_MILLIS = 100;
int DELAY =10;
int GAME_TIME = 100;

long OFFSET_SECS_FROM_START(int duration_secs){
    return (START_MILLIS + duration_secs*1000);
}

void test_initial_state(void){    
    game_clock_configure(&cs,DELAY,GAME_TIME);

    TEST_ASSERT_EQUAL(ClockState::NOT_STARTED,cs.clockState);
    TEST_ASSERT_EQUAL(START_MILLIS,cs.lastupdate_time_millis);
    TEST_ASSERT_EQUAL(DELAY,cs.start_delay_secs);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_duration_secs);
    TEST_ASSERT_EQUAL(0,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.time_to_display_secs);
}

void test_just_before_delay(){
    game_clock_configure(&cs,DELAY,GAME_TIME);
    game_clock_start(&cs,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(9);
    game_clock_update(&cs,secs_from_start);

    TEST_ASSERT_EQUAL(ClockState::COUNTING_TO_START,cs.clockState);
    TEST_ASSERT_EQUAL(0,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(1,cs.secs_till_start);
    TEST_ASSERT_EQUAL(DELAY-secs_from_start,cs.time_to_display_secs);

}

void test_just_after_delay(){
    game_clock_configure(&cs,DELAY,GAME_TIME);
    game_clock_start(&cs,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(11);
    game_clock_update(&cs,secs_from_start);

    TEST_ASSERT_EQUAL(ClockState::COUNTING_TO_START,cs.clockState);
    TEST_ASSERT_EQUAL(secs_from_start-DELAY,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME-secs_from_start-DELAY,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.secs_till_start);
    TEST_ASSERT_EQUAL(GAME_TIME-secs_from_start-DELAY,cs.time_to_display_secs);
}

void test_just_before_over(){
    game_clock_configure(&cs,DELAY,GAME_TIME);
    game_clock_start(&cs,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(99);
    game_clock_update(&cs,secs_from_start);

    TEST_ASSERT_EQUAL(ClockState::IN_PROGRESS,cs.clockState);
    TEST_ASSERT_EQUAL(secs_from_start-DELAY,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME-secs_from_start-DELAY,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.secs_till_start);
    TEST_ASSERT_EQUAL(GAME_TIME-secs_from_start-DELAY,cs.time_to_display_secs);

}

void test_just_after_over(){
    game_clock_configure(&cs,DELAY,GAME_TIME);
    game_clock_start(&cs,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(101);
    game_clock_update(&cs,secs_from_start);

    TEST_ASSERT_EQUAL(ClockState::OVER,cs.clockState);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(0,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.secs_till_start);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.time_to_display_secs);

}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();

    RUN_TEST(test_initial_state);
    RUN_TEST(test_just_before_delay);
    RUN_TEST(test_just_after_delay);
    RUN_TEST(test_just_before_over);
    RUN_TEST(test_just_after_over);
    UNITY_END();
}

void loop() {
    delay(500);
}
