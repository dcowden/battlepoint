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


MeterSettings base_meters(){
    MeterSettings s;        
    initMeter(&s.leftTop.meter,"topLeds",topLeds,0,3);
    initMeter(&s.leftBottom.meter,"bottomLeds",bottomLeds,0,3);
    initMeter(&s.rightTop.meter,"rightTop",topLeds,4,7);
    initMeter(&s.rightBottom.meter,"rightBottom",bottomLeds,4,7);
    initMeter(&s.center.meter,"center",centerLeds,0,7);
    initMeter(&s.left.meter,"left",leftLeds,0,7);
    initMeter(&s.right.meter,"right",rightLeds,0,7);
    return s;
}

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
    updateGame(&gameState, sensorState, gameSettings, (Clock*)(&gameClock));
    long current_time_millis = gameClock.milliseconds();
    updateLeds(&gameState,current_time_millis);

    sensorState.leftScan.was_hit = false;
    sensorState.rightScan.was_hit = false;    
}

void setup_game(GameType gt){
    gameSettings.gameType = gt;
    gameSettings.hits.to_win = 8;
    gameSettings.hits.victory_margin =2;
    gameSettings.timed.max_duration_seconds= 60;
    gameSettings.timed.max_overtime_seconds = 20;
    gameState = startGame(gameSettings, &gameClock,base_meters());
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

    //red two hits.
    add_seconds(5);
    red_hit();
    update();

    CRGB ONE_RED_HITS [VERTICAL_LED_SIZE] = {CRGB::Red, CRGB::Black,CRGB::Black,CRGB::Black,
                                        CRGB::Black,CRGB::Black, CRGB::Black, CRGB::Black };

    Log.infoln("Check Red Hits Meter");
    ASSERT_LEDS_EQUAL(ONE_RED_HITS,leftLeds,VERTICAL_LED_SIZE,"One Red Hit");

    Log.infoln("Check Blue Hits.");
    ASSERT_LEDS_EQUAL(ALL_BLACK,rightLeds,VERTICAL_LED_SIZE,"No Blue Hits");

    Log.infoln("Check Team Meters");
    ASSERT_LEDS_EQUAL(ALL_BLACK,centerLeds,VERTICAL_LED_SIZE,"Center All Black");
    ASSERT_LEDS_EQUAL(TEAM_COLORS,topLeds,VERTICAL_LED_SIZE,"top Team Colors");
    ASSERT_LEDS_EQUAL(TEAM_COLORS,bottomLeds,VERTICAL_LED_SIZE,"bottom team colors");

}

void setup() {
    
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    UNITY_BEGIN();

    //simple meter tests
    //RUN_TEST(test_game_setup);
    RUN_TEST(test_one_team_wins_no_ot);


    UNITY_END();

}
void loop() {
    delay(500);
}