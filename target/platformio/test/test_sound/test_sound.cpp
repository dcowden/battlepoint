#include <Arduino.h>
#include <sound.cpp>
#include <unity.h>

#define BOGUS_TIME 12345

void test_init(void){    
    sound_init_for_testing();
}

void test_game_sounds_trigger(void){
    sound_init_for_testing();
    
    const int GAME_TIME_SECS = 100;
    const int GAME_COUNTDOWN_SECS = 20;
    const int UPDATE_INTERVAL_MS = 200;
    long current_time_millis=0;
    long END_OF_GAME_MS = (GAME_TIME_SECS+GAME_COUNTDOWN_SECS)*1000;

    while (current_time_millis < END_OF_GAME_MS ){
        int current_secs = (current_time_millis/1000) - GAME_COUNTDOWN_SECS;
        int secs_remaining = GAME_TIME_SECS - current_secs;
        if ( current_secs < 0 ){
            sound_gametime_update(current_secs,current_time_millis);
        }
        else{
            sound_gametime_update(secs_remaining,current_time_millis);
        }
        
        current_time_millis += UPDATE_INTERVAL_MS;        
    }

    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_10SEC) == 1);
    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_20SEC) == 1);    
    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_2MIN) == 0);
    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_20SEC) == 1);
    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_30SEC) == 1);
    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_5SEC) == 1);
    TEST_ASSERT_TRUE(sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_6SEC) == 1);        
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_game_sounds_trigger);
    UNITY_END();
}

void loop() {
    delay(500);
}