#include <Arduino.h>
#include <LedMeter.h>
#include <unity.h>
#include <game.h>
#include <FastLED.h>
#include <Clock.h>

/**
 * In this integration test, the game logic, update loop, and meter updates are tested
 * the only thing not included is the target scanning.
 * 
 * */
#define VERTICAL_LED_SIZE 8
#define HORIONTAL_LED_SIZE 4

CRGB ALL_BLUE[VERTICAL_LED_SIZE] = {CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue,
                            CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue };

CRGB ALL_BLACK[VERTICAL_LED_SIZE] = {CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black,
                            CRGB::Black, CRGB::Black,CRGB::Black, CRGB::Black };

CRGB ALL_RED[VERTICAL_LED_SIZE] = {CRGB::Red, CRGB::Red,CRGB::Red, CRGB::Red,
                            CRGB::Red, CRGB::Red,CRGB::Red, CRGB::Red };

GameState gameState;
GameSettings gameSettings;
SensorState sensorState;

TestClock gameClock = TestClock();

//top and bottom leds share one LED strip, in different segments
CRGB leftLeds[VERTICAL_LED_SIZE];
CRGB centerLeds[VERTICAL_LED_SIZE];
CRGB rightLeds[VERTICAL_LED_SIZE];
CRGB topLeds[2* HORIONTAL_LED_SIZE];
CRGB bottomLeds[2* HORIONTAL_LED_SIZE];


MeterSettings base_meters(){
    MeterSettings s;        
    initMeter(&s.leftTop.meter,0,3);
    initMeter(&s.leftBottom.meter,0,3);
    initMeter(&s.rightTop.meter,4,7);
    initMeter(&s.rightBottom.meter,4,7);
    initMeter(&s.center.meter,0,7);
    initMeter(&s.left.meter,0,7);
    initMeter(&s.right.meter,0,7);
    return s;
}

void ASSERT_LEDS_EQUAL(CRGB* expected, CRGB* actual, int num_leds){
    CRGB* actual_ptr = actual;
    for (int i=0;i<num_leds;i++,expected++,actual_ptr++){
        TEST_ASSERT_EQUAL(*expected,*actual);
    }
}

void add_hits(int red_hits, int blue_hits){
    sensorState.leftScan.was_hit = true;
    sensorState.leftScan.hit_millis = gameClock.milliseconds();
     sensorState.rightScan.was_hit = true;
    sensorState.rightScan.hit_millis = gameClock.milliseconds();   
}

void updateMeters(){
    //TODO duplicated in main.cpp, need to re-use this
  MeterSettings ms = gameState.meters;
  updateController(leftLeds, ms.left, gameClock.milliseconds());
  updateController(centerLeds, ms.center, gameClock.milliseconds());
  updateController(rightLeds, ms.right, gameClock.milliseconds());
  updateController(topLeds, ms.leftTop, gameClock.milliseconds());
  updateController(topLeds, ms.rightTop, gameClock.milliseconds());
  updateController(bottomLeds, ms.leftBottom, gameClock.milliseconds());
  updateController(bottomLeds, ms.rightBottom , gameClock.milliseconds());
}

void setup_game(GameType gt){
    gameSettings.gameType = gt;
    gameSettings.hits.to_win = 10;
    gameSettings.hits.victory_margin =2;
    gameSettings.timed.max_duration_seconds= 60;
    gameSettings.timed.max_overtime_seconds = 20;

    gameState = startGame(gameSettings, &gameClock,base_meters());
    gameClock.setTime(0);
    updateGame(&gameState, sensorState, gameSettings, (Clock*)(&gameClock));
}

void test_one_team_wins_no_ot(){
    setup_game(GameType::GAME_TYPE_KOTH_FIRST_TO_HITS);
    ASSERT_LEDS_EQUAL(ALL_BLACK,leftLeds,VERTICAL_LED_SIZE);


}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    UNITY_BEGIN();

    //simple meter tests
    RUN_TEST(test_one_team_wins_no_ot);


    UNITY_END();

}
void loop() {
    delay(500);
}