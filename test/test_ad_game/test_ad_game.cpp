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
};

void test_ad_game_initial_state(void){
    GameOptions go = std_game_options();    
    ADGame game = ADGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    game.update();
    TEST_ASSERT_FALSE(game.isRunning() );
    TEST_ASSERT_EQUAL( Team::NOBODY, game.getWinner() );
    TEST_ASSERT_EQUAL(0,game.getSecondsElapsed());
    TEST_ASSERT_EQUAL(go.timeLimitSeconds,game.getRemainingSeconds() );
    TEST_ASSERT_EQUAL(Team::RED, testControlPoint.getOwner());
    TEST_ASSERT_EQUAL(100, owner.getValue());
    TEST_ASSERT_EQUAL(0, capture.getValue());
}

void test_ad_game_keeps_time(){
    GameOptions go = std_game_options();    
    ADGame game = ADGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
   
    game.start();  
    TEST_ASSERT_EQUAL( Team::NOBODY, game.getWinner() );
    TEST_ASSERT_TRUE(game.isRunning() );
    TEST_ASSERT_EQUAL(go.timeLimitSeconds, game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(Team::RED, testControlPoint.getOwner() );

    int CAP_TIME=5;
    tc.addSeconds(CAP_TIME);
    game.update();
    TEST_ASSERT_INT_WITHIN(1,CAP_TIME,game.getSecondsElapsed());
    
    TEST_ASSERT_EQUAL(go.timeLimitSeconds - CAP_TIME, game.getRemainingSeconds());
    TEST_ASSERT_EQUAL(CAP_TIME, game.getAccumulatedSeconds(Team::RED));
    TEST_ASSERT_EQUAL(0, game.getAccumulatedSeconds(Team::BLU));

}

void test_ad_game_ends_after_capture(){
    //in a cp 
    GameOptions go = std_game_options();    
    ADGame game = ADGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    game.start();    
    testControlPoint.setOwner(Team::BLU);
    tc.addSeconds(1);
    game.update();

    TEST_ASSERT_EQUAL(Team::BLU, game.getWinner());
    TEST_ASSERT_FALSE(game.isRunning());
    TEST_ASSERT_EQUAL(100, owner.getValue());
    TEST_ASSERT_TRUE(testControlPoint.isOwnedBy(Team::BLU));

    TEST_ASSERT_EQUAL(0,game.getRemainingSeconds());

}

void test_ad_game_ends_after_time_limit(){
    //in a cp 
    GameOptions go = std_game_options();    
    ADGame game = ADGame();
    TestClock tc = TestClock();
    game.init(&testControlPoint, go, &em, &owner, &capture, &timer1, &timer2, &tc);
    game.start();    

    tc.addSeconds(go.timeLimitSeconds +1);
    game.update();

    TEST_ASSERT_EQUAL(Team::RED, game.getWinner());
    TEST_ASSERT_FALSE(game.isRunning());
    TEST_ASSERT_EQUAL(100, owner.getValue());
    TEST_ASSERT_TRUE(testControlPoint.isOwnedBy(Team::RED));

    TEST_ASSERT_EQUAL(0,game.getRemainingSeconds());

}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_ad_game_initial_state);
    RUN_TEST(test_ad_game_keeps_time);
    RUN_TEST(test_ad_game_ends_after_capture);
    RUN_TEST(test_ad_game_ends_after_time_limit);
    
    UNITY_END();
}

void loop() {
    delay(500);
}
