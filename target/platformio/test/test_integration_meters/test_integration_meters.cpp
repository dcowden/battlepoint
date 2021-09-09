#include <Arduino.h>
#include <LedMeter.h>
#include <unity.h>
#include <game.h>
#include <FastLED.h>
#include <Clock.h>
#include <ArduinoLog.h>

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

CRGB TEAM_COLORS[VERTICAL_LED_SIZE] = {CRGB::Red, CRGB::Red,CRGB::Red, CRGB::Red,
                            CRGB::Blue, CRGB::Blue,CRGB::Blue, CRGB::Blue };

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

LedMeter leftTopMeter;
LedMeter leftBottomMeter;
LedMeter rightTopMeter;
LedMeter rightBottomMeter;
LedMeter centerMeter;
LedMeter leftMeter;
LedMeter rightMeter;

MeterSettings meters;
void ASSERT_LEDS_EQUAL(CRGB* expected, CRGB* actual, int num_leds, const char* message){
    CRGB* actual_ptr = actual;
    Log.infoln("Led Check: %s",message);
    Serial.println("LED E A");
    for (int i=0;i<num_leds;i++,expected++,actual_ptr++){
        Serial.print(i);Serial.print("   ");Serial.print(*expected);Serial.print(" ");Serial.println(*actual);
        TEST_ASSERT_EQUAL(*expected,*actual_ptr);
    }
}

void red_hit(){
    sensorState.leftScan.was_hit = true;
    sensorState.leftScan.hit_millis = gameClock.milliseconds();

}
void blue_hit(){
    sensorState.rightScan.was_hit = true;
    sensorState.rightScan.hit_millis = gameClock.milliseconds();   
}

void add_seconds(long seconds){
    gameClock.addSeconds(seconds);
}
void update(){
    updateGame(&gameState, &sensorState, gameSettings, (Clock*)(&gameClock));
    updateMeters(&gameState, &gameSettings, &meters);
    long current_time_millis = gameClock.milliseconds();
    updateLeds(&meters,current_time_millis);

    sensorState.leftScan.was_hit = false;
    sensorState.rightScan.was_hit = false;    
}

void setupMeters(){
  //probably this should be factored, since is necessary for both the tests and any reasonable user
  meters.leftTop.meter = &leftTopMeter;
  meters.leftBottom.meter = &leftBottomMeter;
  meters.rightTop.meter = &rightTopMeter;
  meters.rightBottom.meter = &rightBottomMeter;
  meters.center.meter = &centerMeter;
  meters.left.meter  = &leftMeter;
  meters.right.meter = &rightMeter;

  initMeter(meters.leftTop.meter,"leftTop",topLeds,0,3);
  initMeter(meters.leftBottom.meter,"leftBottom",bottomLeds,0,3);
  initMeter(meters.rightTop.meter,"rightTop",topLeds,4,7);
  initMeter(meters.rightBottom.meter,"rightBottom",bottomLeds,4,7);
  initMeter(meters.center.meter,"center",centerLeds,0,7);
  initMeter(meters.left.meter,"left",leftLeds,0,7);
  initMeter(meters.right.meter,"right",rightLeds,0,7);
}
void setup_game(GameType gt){

    gameSettings.gameType = gt;
    gameSettings.hits.to_win = 8;
    gameSettings.hits.victory_margin =2;
    gameSettings.timed.max_duration_seconds= 60;
    gameSettings.timed.max_overtime_seconds = 20;
    startGame(&gameState, &gameSettings, &gameClock);
    gameClock.setTime(0);
    update();
}
void test_game_setup(){

    setup_game(GameType::GAME_TYPE_KOTH_FIRST_TO_HITS);
    ASSERT_LEDS_EQUAL(ALL_BLACK,leftLeds,VERTICAL_LED_SIZE,"Left Black");
    ASSERT_LEDS_EQUAL(ALL_BLACK,rightLeds,VERTICAL_LED_SIZE,"Right Black");
    ASSERT_LEDS_EQUAL(ALL_BLACK,centerLeds,VERTICAL_LED_SIZE,"Center Black");
    ASSERT_LEDS_EQUAL(TEAM_COLORS,topLeds,VERTICAL_LED_SIZE,"top Team Colors");
    ASSERT_LEDS_EQUAL(TEAM_COLORS,bottomLeds,VERTICAL_LED_SIZE,"bottom team colors");
}

void test_one_team_wins_no_ot(){

    setup_game(GameType::GAME_TYPE_KOTH_FIRST_TO_HITS);
    Log.traceln("Setup Complete");

    add_seconds(5);
    red_hit();
    update();
    add_seconds(5);
    red_hit();
    blue_hit();
    update();

    CRGB TWO_RED_HITS [VERTICAL_LED_SIZE] = {CRGB::Red, CRGB::Red,CRGB::Black,CRGB::Black,
                                        CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black };

    CRGB ONE_BLUE_HIT [VERTICAL_LED_SIZE] = {CRGB::Blue, CRGB::Black,CRGB::Black,CRGB::Black,
                                        CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black };
                                   
    Log.infoln("Check Red Hits Meter");
    ASSERT_LEDS_EQUAL(TWO_RED_HITS,leftLeds,VERTICAL_LED_SIZE,"Two Red Hits");

    Log.infoln("Check Blue Hits.");
    ASSERT_LEDS_EQUAL(ONE_BLUE_HIT,rightLeds,VERTICAL_LED_SIZE,"One Blue Hit");

    Log.infoln("Check Team Meters");
    ASSERT_LEDS_EQUAL(ALL_BLACK,centerLeds,VERTICAL_LED_SIZE,"Center All Black");
    ASSERT_LEDS_EQUAL(TEAM_COLORS,topLeds,VERTICAL_LED_SIZE,"top Team Colors");
    ASSERT_LEDS_EQUAL(TEAM_COLORS,bottomLeds,VERTICAL_LED_SIZE,"bottom team colors");

    TEST_ASSERT_EQUAL_MESSAGE(meters.rightBottom.meter->flash_interval_millis, FLASH_NONE,"rightBottom should not flash");
    TEST_ASSERT_EQUAL_MESSAGE(meters.rightTop.meter->flash_interval_millis, FLASH_NONE,"rightTop should not flash");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftBottom.meter->flash_interval_millis, FLASH_NONE,"leftBottom should not flash ");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftTop.meter->flash_interval_millis, FLASH_NONE,"leftTop should not flash");     

}

void test_one_team_overtime(){

    setup_game(GameType::GAME_TYPE_KOTH_FIRST_TO_HITS);
    Log.traceln("Setup Complete");

    for(int i=0;i<7;i++){
        add_seconds(5);
        red_hit();
        add_seconds(5);
        blue_hit();
        update();
    }

    add_seconds(5);
    red_hit();
    update();

    CRGB BLUE_HITS [VERTICAL_LED_SIZE] = {CRGB::Blue, CRGB::Blue,CRGB::Blue,CRGB::Blue,
                                        CRGB::Blue,CRGB::Blue, CRGB::Blue, CRGB::Black };

    Log.infoln("Check Red Hits Meter");
    ASSERT_LEDS_EQUAL(ALL_RED,leftLeds,VERTICAL_LED_SIZE,"8 Red Hits");

    Log.infoln("Check Blue Hits.");
    ASSERT_LEDS_EQUAL(BLUE_HITS,rightLeds,VERTICAL_LED_SIZE,"7 Blue Hits");

    Log.infoln("Check Team Meters");
    ASSERT_LEDS_EQUAL(ALL_BLACK,centerLeds,VERTICAL_LED_SIZE,"Center All Black");
    //ASSERT_LEDS_EQUAL(TEAM_COLORS,topLeds,VERTICAL_LED_SIZE,"top Team Colors");
    //ASSERT_LEDS_EQUAL(TEAM_COLORS,bottomLeds,VERTICAL_LED_SIZE,"bottom team colors");

    TEST_ASSERT_EQUAL_MESSAGE(meters.rightBottom.meter->flash_interval_millis, FLASH_FAST,"rightBottom should flash slow");
    TEST_ASSERT_EQUAL_MESSAGE(meters.rightTop.meter->flash_interval_millis, FLASH_FAST,"rightTop should flash");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftBottom.meter->flash_interval_millis, FLASH_SLOW,"leftBottom should flash fast");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftTop.meter->flash_interval_millis, FLASH_SLOW,"leftTop should flash fast");    
}

void setup() {
    setupMeters();
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();

    //simple meter tests
    //RUN_TEST(test_game_setup);
    RUN_TEST(test_one_team_wins_no_ot);
    //RUN_TEST(test_one_team_overtime);

    UNITY_END();

}
void loop() {
    delay(500);
}