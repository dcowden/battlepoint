#include <Arduino.h>
#include <Teams.h>
#include <unity.h>
#include <Game.h>
#include <Clock.h>

TestControlPoint testControlPoint = TestControlPoint();
SimpleMeter owner = SimpleMeter();
SimpleMeter capture = SimpleMeter();
SimpleMeter timer1 = SimpleMeter();
SimpleMeter timer2 = SimpleMeter();
TestEventManager em = TestEventManager();

GameOptions koth_game_options(void){
    GameOptions go;
    go.timeLimitSeconds=2;
    go.startDelaySeconds=0;
    go.mode = GameMode::KOTH;
    go.captureSeconds=1;
    go.captureButtonThresholdSeconds=1;
    return go;    
}

void test_koth_game_initial_state(void){
    GameOptions go = koth_game_options();    
    KothGame koth = KothGame();
    TestClock tc = TestClock();
    koth.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    TEST_ASSERT_FALSE(koth.isRunning() );
    TEST_ASSERT_EQUAL( Team::NOBODY, koth.getWinner() );
    TEST_ASSERT_EQUAL(0,koth.getSecondsElapsed());
    TEST_ASSERT_EQUAL(go.timeLimitSeconds,koth.getRemainingSeconds() );
    TEST_ASSERT_EQUAL(0, owner.getValue());
    TEST_ASSERT_EQUAL(0, capture.getValue());

}

void test_koth_game_keeps_time(){
    GameOptions go = koth_game_options();    
    KothGame koth = KothGame();
    TestClock tc = TestClock();
    koth.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    testControlPoint.setOwner(Team::NOBODY);
    koth.start();  
    TEST_ASSERT_EQUAL( Team::NOBODY, koth.getWinner() );
    TEST_ASSERT_TRUE(koth.isRunning() );
    TEST_ASSERT_EQUAL(go.timeLimitSeconds, koth.getRemainingSeconds());
    tc.addTime(1000);
    koth.update();
    TEST_ASSERT_INT_WITHIN(1,1,koth.getSecondsElapsed());

    //in a koth game with no owner, time remaining stays at the time limit
    TEST_ASSERT_EQUAL(go.timeLimitSeconds, koth.getRemainingSeconds());
    TEST_ASSERT_EQUAL(0, koth.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(0, koth.getAccumulatedSeconds(Team::BLU));

    //blue capture
    testControlPoint.setCapturingTeam(Team::BLU);
    tc.addTime(2000);
    koth.update();
}

void test_koth_game_ends_after_capture(){
    GameOptions go = koth_game_options();    
    KothGame koth = KothGame();
    TestClock tc = TestClock();
    koth.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    koth.start();
    testControlPoint.setOwner(Team::BLU);
    koth.update();    
    tc.addTime(1100);
    koth.update();
    TEST_ASSERT_EQUAL(0, koth.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(1, koth.getAccumulatedSeconds(Team::BLU));

    tc.addTime(1000);
    koth.update();
    TEST_ASSERT_EQUAL(Team::BLU, koth.getWinner());
    TEST_ASSERT_EQUAL(0, koth.getRemainingSeconds());

    TEST_ASSERT_EQUAL(100, owner.getValue());
    TEST_ASSERT_TRUE(testControlPoint.isOwnedBy(Team::BLU));
    TEST_ASSERT_EQUAL(Team::BLU, koth.getWinner());

    TEST_ASSERT_EQUAL(go.timeLimitSeconds, koth.getRemainingSecondsForTeam(Team::RED));
    TEST_ASSERT_EQUAL(0, koth.getRemainingSecondsForTeam(Team::BLU));
    
    TEST_ASSERT_EQUAL(0, koth.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(2, koth.getAccumulatedSeconds(Team::BLU));

    TEST_ASSERT_EQUAL(go.timeLimitSeconds, timer2.getValue());
    TEST_ASSERT_EQUAL(0, timer1.getValue());

}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_koth_game_initial_state);
    RUN_TEST(test_koth_game_keeps_time);
    RUN_TEST(test_koth_game_ends_after_capture);
    
    UNITY_END();
}

void loop() {
    delay(500);
}
