#include <Arduino.h>
#include <Teams.h>
#include <unity.h>
#include <Game.h>

TestControlPoint testControlPoint = TestControlPoint();
SimpleMeter owner = SimpleMeter();
SimpleMeter capture = SimpleMeter();
SimpleMeter timer1 = SimpleMeter();
SimpleMeter timer2 = SimpleMeter();
TestEventManager em = TestEventManager();


void test_koth_game_initial_state(void){
    GameOptions go;
    go.timeLimitSeconds=2;
    go.startDelaySeconds=1;
    go.mode = GameMode::KOTH;
    go.captureSeconds=2;
    go.captureButtonThresholdSeconds=1;
    
    KothGame koth = KothGame();
    koth.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2);
    TEST_ASSERT_FALSE(koth.isRunning() );
    TEST_ASSERT_EQUAL( Team::NOBODY, koth.getWinner() );
    TEST_ASSERT_EQUAL(0,koth.getSecondsElapsed());
    TEST_ASSERT_EQUAL(go.timeLimitSeconds,koth.getRemainingSeconds() );
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_koth_game_initial_state);
    //RUN_TEST(test_team_chars);
    UNITY_END();
}

void loop() {
    delay(500);
}
