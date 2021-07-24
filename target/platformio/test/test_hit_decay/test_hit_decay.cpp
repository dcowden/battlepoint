#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>


GameSettings settings;
GameState currentState;

void rollTimeForward(long milliseconds_start, int milliseconds_interval, long milliseconds_end){
    int intervals = (milliseconds_end - milliseconds_start) / milliseconds_interval;
    long current_time_millis = milliseconds_start;
    for(int i=0;i<intervals;i++){
        current_time_millis += milliseconds_interval;
        updateGameTime(&currentState,settings,current_time_millis);
        applyHitDecay(&currentState,settings,current_time_millis);
    }
}

void test_decay_less_than_hits_total(){
    const int INITIAL_HITS=10;
    currentState.ownership.capturing= Team::RED;
    currentState.ownership.last_decay_millis=0;
    currentState.ownership.capture_hits=INITIAL_HITS;    

    settings.capture.capture_decay_rate_secs_per_hit = 2;
    rollTimeForward(0,100,5000);

    TEST_ASSERT_EQUAL_INT(INITIAL_HITS-2,currentState.ownership.capture_hits);
}

void test_decay_hits_below_zero(){
    const int INITIAL_HITS=1;
    currentState.ownership.capturing= Team::RED;  
    currentState.ownership.last_decay_millis=0;
    currentState.ownership.capture_hits=INITIAL_HITS; 
    settings.capture.capture_decay_rate_secs_per_hit = 2;
    rollTimeForward(1000,100,5000);

    TEST_ASSERT_EQUAL_INT(0,currentState.ownership.capture_hits);
}

void test_no_decay_with_zero_decay_rate(){
    const int INITIAL_HITS=1;
    currentState.ownership.capturing= Team::RED;  
    currentState.ownership.last_decay_millis=0;
    currentState.ownership.capture_hits=INITIAL_HITS; 
    settings.capture.capture_decay_rate_secs_per_hit = 0;
    rollTimeForward(1000,100,50000);

    TEST_ASSERT_EQUAL_INT(INITIAL_HITS,currentState.ownership.capture_hits);
}




void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_INFO, &Serial, true);
    UNITY_BEGIN();
    RUN_TEST(test_decay_less_than_hits_total);
    RUN_TEST(test_decay_hits_below_zero);
    RUN_TEST(test_no_decay_with_zero_decay_rate);

    UNITY_END();

}
void loop() {
    delay(500);
}