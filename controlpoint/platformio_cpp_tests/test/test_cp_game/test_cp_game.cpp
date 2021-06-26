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
    go.mode = GameMode::CP;
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
   
    game.start();  
    TEST_ASSERT_EQUAL( Team::NOBODY, game.getWinner() );
    TEST_ASSERT_TRUE(game.isRunning() );
    TEST_ASSERT_EQUAL(go.timeLimitSeconds, game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(Team::NOBODY, testControlPoint.getOwner() );

    int CAP_TIME=5;
    tc.addSeconds(CAP_TIME);
    testControlPoint.setOwner(Team::RED);
    game.update();
    TEST_ASSERT_INT_WITHIN(1,CAP_TIME,game.getSecondsElapsed());
    
    TEST_ASSERT_EQUAL(go.timeLimitSeconds - CAP_TIME, game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(CAP_TIME, game.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(0, game.getAccumulatedSeconds(Team::BLU));

}

void test_cp_game_ends_after_time_limit(){
    //in a cp 
    GameOptions go = std_game_options();    
    CPGame game = CPGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    game.start();
    int BLUE_CAP_TIME=5;
    int RED_CAP_TIME=20; 
    
    testControlPoint.setOwner(Team::BLU);
    tc.addSeconds(BLUE_CAP_TIME);
    game.update();

    testControlPoint.setOwner(Team::RED);
    tc.addSeconds(RED_CAP_TIME);
    game.update();

    TEST_ASSERT_EQUAL(RED_CAP_TIME, game.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(BLUE_CAP_TIME, game.getAccumulatedSeconds(Team::BLU));

    TEST_ASSERT_EQUAL(Team::RED, game.getWinner());
    TEST_ASSERT_FALSE(game.isRunning());
    TEST_ASSERT_EQUAL(100, owner.getValue());
    TEST_ASSERT_TRUE(testControlPoint.isOwnedBy(Team::RED));
    TEST_ASSERT_EQUAL(Team::RED, game.getWinner());

    TEST_ASSERT_EQUAL(0,game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(RED_CAP_TIME, timer2.getValue());
    TEST_ASSERT_EQUAL(BLUE_CAP_TIME, timer1.getValue());

}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_cp_game_initial_state);
    RUN_TEST(test_cp_game_keeps_time);
    RUN_TEST(test_cp_game_ends_after_time_limit);
    
    UNITY_END();
}

void loop() {
    delay(500);
}
