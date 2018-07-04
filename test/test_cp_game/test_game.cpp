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

GameOptions std_game_options(void){
    GameOptions go;
    go.timeLimitSeconds=20;
    go.startDelaySeconds=0;
    go.mode = GameMode::KOTH;
    go.captureSeconds=5;
    go.captureButtonThresholdSeconds=1;
    return go;    
}
void test_cp_game_initial_state(void){
    GameOptions go = std_game_options();    
    CPGame cp = CPGame();
    TestClock tc = TestClock();
    cp.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    TEST_ASSERT_FALSE(cp.isRunning() );
    TEST_ASSERT_EQUAL( Team::NOBODY, cp.getWinner() );
    TEST_ASSERT_EQUAL(0,cp.getSecondsElapsed());
    TEST_ASSERT_EQUAL(go.timeLimitSeconds,cp.getRemainingSeconds() );
    TEST_ASSERT_EQUAL(0, owner.getValue());
    TEST_ASSERT_EQUAL(0, capture.getValue());
}

void test_cp_game_keeps_time(){
    GameOptions go = std_game_options();    
    CPGame game = CPGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    testControlPoint.setOwner(Team::NOBODY);
    game.start();  
    TEST_ASSERT_EQUAL( Team::NOBODY, game.getWinner() );
    TEST_ASSERT_TRUE(game.isRunning() );
    TEST_ASSERT_EQUAL(go.timeLimitSeconds, game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(Team::RED, testControlPoint.getOwner() );
    tc.addTime(1000);
    game.update();
    TEST_ASSERT_INT_WITHIN(1,1,game.getSecondsElapsed());
    
    TEST_ASSERT_EQUAL(go.timeLimitSeconds - 1, game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(0, game.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(0, game.getAccumulatedSeconds(Team::BLU));

}

void test_cp_game_ends_after_capture(){
    GameOptions go = std_game_options();    
    CPGame game = CPGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    game.start();
     
    tc.addTime(1100);
    testControlPoint.setOwner(Team::BLU);
    game.update();
    TEST_ASSERT_EQUAL(0, game.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(1, game.getAccumulatedSeconds(Team::BLU));

    tc.addTime(1000);
    game.update();
    TEST_ASSERT_EQUAL(Team::BLU, game.getWinner());
    TEST_ASSERT_EQUAL(0, game.getRemainingSeconds());

    TEST_ASSERT_EQUAL(100, owner.getValue());
    TEST_ASSERT_TRUE(testControlPoint.isOwnedBy(Team::BLU));
    TEST_ASSERT_EQUAL(Team::BLU, game.getWinner());

    TEST_ASSERT_EQUAL(game.getRemainingSeconds(), go.timeLimitSeconds - 2);
    TEST_ASSERT_EQUAL(game.getRemainingSeconds(), timer2.getValue());
    TEST_ASSERT_EQUAL(game.getRemainingSeconds(), timer1.getValue());

}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    //RUN_TEST(test_cp_game_initial_state);
    //RUN_TEST(test_cp_game_keeps_time);
    //RUN_TEST(test_cp_game_ends_after_capture);
    
    UNITY_END();
}

void loop() {
    delay(500);
}
