#include <RespawnTimer.h>
#include <unity.h>
#include<Arduino.h>
#include <Clock.h>



TestClock clock;
RespawnTimer simpleTimer(&clock,0);

TimerConfig standardConfig = { 
    5000, //durationMillis
    2000, //immientMillis
    2000, //afterFinishMillis
    1000, //afterStartMillis
};

int capturedState = -1;
int stateChangeCount = 0;
void myStateChange(RespawnTimerState newState,int id){
    capturedState = newState;
}

void stateChangeCounter(RespawnTimerState newState, int id){
    stateChangeCount++;
}

void test_negative_elapsed_gives_idle(void){
    TEST_ASSERT_EQUAL(RespawnTimerState::IDLE, computeStateForTimer(standardConfig, -1 )); //negative number should be idle
}

void test_valid_timer_states(void){
    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWN_START, computeStateForTimer(standardConfig, 0 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWN_START, computeStateForTimer(standardConfig, 999 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWNING, computeStateForTimer(standardConfig, 1001 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::IMMINENT, computeStateForTimer(standardConfig, 3001 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::IMMINENT, computeStateForTimer(standardConfig, 4999 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::FINISHED, computeStateForTimer(standardConfig, 5000 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::FINISHED, computeStateForTimer(standardConfig, 5001 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::FINISHED, computeStateForTimer(standardConfig, 5999 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::FINISHED, computeStateForTimer(standardConfig, 6999 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::IDLE, computeStateForTimer(standardConfig, 7000 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::IDLE, computeStateForTimer(standardConfig, 7001 )); 
    TEST_ASSERT_EQUAL(RespawnTimerState::IDLE, computeStateForTimer(standardConfig, 90000 )); 
}

void test_init_starts_disabled(void){
    TEST_ASSERT_TRUE( simpleTimer.state() == RespawnTimerState::IDLE);
    TEST_ASSERT_TRUE( simpleTimer.isAvailable() );
}

void test_started_timer_is_respawning(){

    clock.setTime(0);
    simpleTimer.start(standardConfig);

    clock.setTime(1001); //standardConfig is RESPAWN_START until > 1 second
    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWNING, simpleTimer.state());
    TEST_ASSERT_FALSE(simpleTimer.isAvailable());

    clock.setTime(4000);
    TEST_ASSERT_EQUAL(RespawnTimerState::IMMINENT, simpleTimer.state());

    clock.setTime(5100);
    TEST_ASSERT_EQUAL(RespawnTimerState::FINISHED, simpleTimer.state());        
    TEST_ASSERT_TRUE( simpleTimer.isAvailable() );

    clock.setTime(8000);
    TEST_ASSERT_EQUAL(RespawnTimerState::IDLE, simpleTimer.state());
    simpleTimer.start(standardConfig);

    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWN_START, simpleTimer.state());

}

void test_stopping_a_timer(void){
    clock.setTime(0);
    simpleTimer.start(standardConfig); 
    clock.setTime(1000);
    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWNING, simpleTimer.state());    
    simpleTimer.stop();
    TEST_ASSERT_EQUAL(RespawnTimerState::IDLE, simpleTimer.state());           
}

void test_callback_catches_start(void){
    clock.setTime(0);
    simpleTimer.onStateChange(myStateChange);
    simpleTimer.start(standardConfig); 
    TEST_ASSERT_EQUAL(RespawnTimerState::RESPAWN_START,capturedState);
}

void test_callback_catches_finish(void){
    clock.setTime(0);
    simpleTimer.onStateChange(stateChangeCounter);
    simpleTimer.start(standardConfig);
    clock.setTime(12000);
    simpleTimer.state(); 
    TEST_ASSERT_EQUAL(2,stateChangeCount);
}


void RUN_UNITY_TESTS() {
    UNITY_BEGIN();
    RUN_TEST(test_negative_elapsed_gives_idle);
    RUN_TEST(test_valid_timer_states);
    RUN_TEST(test_init_starts_disabled);
    RUN_TEST(test_started_timer_is_respawning);
    RUN_TEST(test_stopping_a_timer);
    RUN_TEST(test_callback_catches_start);
    RUN_TEST(test_callback_catches_finish);
    UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    RUN_UNITY_TESTS();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}

#else
#include <ArduinoFake.h>

int main(int argc, char **argv) {
    RUN_UNITY_TESTS();
    return 0;
}

#endif