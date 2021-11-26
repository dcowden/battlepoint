#include <Arduino.h>
#include <ClockDisplay.h>
#include <unity.h>

GameClockState cs;

long START_MILLIS = 100;
int DELAY =10;
int GAME_TIME = 100;

long OFFSET_SECS_FROM_START(int duration_secs){
    return (START_MILLIS + duration_secs*1000);
}

void test_initial_state(void){    
    initGameClock(&cs,DELAY,GAME_TIME,START_MILLIS);

    TEST_ASSERT_FALSE(cs.in_progress);
    TEST_ASSERT_FALSE(cs.is_over);
    TEST_ASSERT_EQUAL(START_MILLIS,cs.lastupdate_time_millis);
    TEST_ASSERT_EQUAL(DELAY,cs.start_delay_secs);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_duration_secs);
    TEST_ASSERT_EQUAL(0,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(ClockColor::BLACK,cs.color);
}

void test_just_before_delay(){
    initGameClock(&cs,DELAY,GAME_TIME,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(9);
    updateGameClock(&cs,secs_from_start);

    TEST_ASSERT_FALSE(cs.in_progress);
    TEST_ASSERT_FALSE(cs.is_over);
    TEST_ASSERT_EQUAL(0,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(1,cs.secs_till_start);
    TEST_ASSERT_EQUAL(ClockColor::WHITE,cs.color);

}

void test_just_after_delay(){
    initGameClock(&cs,DELAY,GAME_TIME,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(11);
    updateGameClock(&cs,secs_from_start);

    TEST_ASSERT_TRUE(cs.in_progress);
    TEST_ASSERT_FALSE(cs.is_over);
    TEST_ASSERT_EQUAL(secs_from_start-DELAY,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME-secs_from_start-DELAY,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.secs_till_start);
    TEST_ASSERT_EQUAL(ClockColor::YELLOW,cs.color);
}

void test_just_before_over(){
    initGameClock(&cs,DELAY,GAME_TIME,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(99);
    updateGameClock(&cs,secs_from_start);

    TEST_ASSERT_TRUE(cs.in_progress);
    TEST_ASSERT_FALSE(cs.is_over);
    TEST_ASSERT_EQUAL(secs_from_start-DELAY,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(GAME_TIME-secs_from_start-DELAY,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.secs_till_start);
    TEST_ASSERT_EQUAL(ClockColor::RED,cs.color);
}

void test_just_after_over(){
    initGameClock(&cs,DELAY,GAME_TIME,START_MILLIS);
    int secs_from_start = OFFSET_SECS_FROM_START(101);
    updateGameClock(&cs,secs_from_start);

    TEST_ASSERT_FALSE(cs.in_progress);
    TEST_ASSERT_TRUE(cs.is_over);
    TEST_ASSERT_EQUAL(GAME_TIME,cs.game_elapsed_secs);
    TEST_ASSERT_EQUAL(0,cs.game_remaining_secs);
    TEST_ASSERT_EQUAL(0,cs.secs_till_start);
    TEST_ASSERT_EQUAL(ClockColor::RED,cs.color);
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
