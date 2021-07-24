#include <Arduino.h>
#include <unity.h>
#include <game.h>
#include <ArduinoLog.h>


GameSettings settings;
GameState state;

void test_owner_time_accululate(){
    state.
}

void preTest(){
    settings.capture.hits_to_capture=10;
    settings.capture.capture_overtime_seconds=30;
    settings.capture.capture_decay_rate_secs_per_hit=1;
}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();
    preTest();
    RUN_TEST(test_in_progress_no_capture_yet);

    UNITY_END();

}
void loop() {
    delay(500);
}