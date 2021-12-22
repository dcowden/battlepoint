#include <Arduino.h>
#include <LedMeter.h>
#include <unity.h>
#include <game.h>
#include <FastLED.h>
#include <Clock.h>
#include <ArduinoLog.h>
#include <sound.h> 
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

long current_time_millis = 0;

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
    TargetHitData hd;
    hd.hits=1;
    applyLeftHits(&gameState,&gameSettings, hd,current_time_millis);
}
void blue_hit(){
    TargetHitData hd;
    hd.hits = 1;
    applyRightHits(&gameState,&gameSettings,hd,current_time_millis);
}

void add_seconds(long seconds){
    current_time_millis += (seconds*1000);
}
void update(){

    updateGame(&gameState, gameSettings, current_time_millis);
    updateMeters(&gameState, &gameSettings, &meters);
    updateLeds(&meters,current_time_millis);
}

void handle_game_start(GameStatus status){
    if ( status == GameStatus::GAME_STATUS_PREGAME){
        sound_play(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED,current_time_millis);
    }
    else{
        sound_play(SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4,current_time_millis);
    }
    
}
void handle_game_end(Team winner){
    sound_play_victory(winner,current_time_millis);
}

void handle_game_seconds_update(int secs, GameStatus status){
    sound_gametime_update(secs,current_time_millis);
}

void setupHandlers(){
    gameState.eventHandler.RemainingSecsHandler=handle_game_seconds_update;
    gameState.eventHandler.StartedHandler=handle_game_start;
    gameState.eventHandler.EndedHandler=handle_game_end;
}

void setupMeters(){
  //probably this should be factored, since is necessary for both the tests and any reasonable user
  meters.leftTop = &leftTopMeter;
  meters.leftBottom = &leftBottomMeter;
  meters.rightTop = &rightTopMeter;
  meters.rightBottom = &rightBottomMeter;
  meters.center = &centerMeter;
  meters.left  = &leftMeter;
  meters.right = &rightMeter;

  initMeter(meters.leftTop,"leftTop",topLeds,0,3);
  initMeter(meters.leftBottom,"leftBottom",bottomLeds,0,3);
  initMeter(meters.rightTop,"rightTop",topLeds,4,7);
  initMeter(meters.rightBottom,"rightBottom",bottomLeds,4,7);
  initMeter(meters.center,"center",centerLeds,0,7);
  initMeter(meters.left,"left",leftLeds,0,7);
  initMeter(meters.right,"right",rightLeds,0,7);
}
void setup_game(GameType gt){
    gameSettings.timed.max_duration_seconds=20;
    gameSettings.timed.countdown_start_seconds=2;
    gameSettings.gameType = gt;
    gameSettings.hits.to_win = 8;
    gameSettings.hits.victory_margin =2;
    gameSettings.timed.max_duration_seconds= 60;
    gameSettings.timed.max_overtime_seconds = 20;
    startGame(&gameState, &gameSettings, current_time_millis);
    current_time_millis = 2000;
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

void test_game_time_progression(){
    current_time_millis=1000;
    gameSettings.timed.max_duration_seconds=20;
    gameSettings.timed.max_overtime_seconds=5;
    gameSettings.timed.ownership_time_seconds=5;
    gameSettings.timed.countdown_start_seconds=6;
    gameSettings.gameType =  GameType::GAME_TYPE_ATTACK_DEFEND;
    startGame(&gameState,&gameSettings,current_time_millis);
    for ( int i=0;i<80;i++){
        current_time_millis += 500;
        update();
    }
    TEST_ASSERT_EQUAL(GameStatus::GAME_STATUS_ENDED, gameState.status);
    TEST_ASSERT_EQUAL(Team::RED, gameState.result.winner);
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_BEGINS_5SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_BEGINS_4SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_BEGINS_3SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_BEGINS_2SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_BEGINS_1SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_10SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_5SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_4SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_3SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_2SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_ANNOUNCER_ENDS_1SEC));
    TEST_ASSERT_EQUAL(1, sound_times_played(SND_SOUNDS_0023_ANNOUNCER_VICTORY));
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

    TEST_ASSERT_EQUAL_MESSAGE(meters.rightBottom->flash_interval_millis, FLASH_NONE,"rightBottom should not flash");
    TEST_ASSERT_EQUAL_MESSAGE(meters.rightTop->flash_interval_millis, FLASH_NONE,"rightTop should not flash");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftBottom->flash_interval_millis, FLASH_NONE,"leftBottom should not flash ");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftTop->flash_interval_millis, FLASH_NONE,"leftTop should not flash");     

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

    TEST_ASSERT_EQUAL_MESSAGE(meters.rightBottom->flash_interval_millis, FLASH_FAST,"rightBottom should flash slow");
    TEST_ASSERT_EQUAL_MESSAGE(meters.rightTop->flash_interval_millis, FLASH_FAST,"rightTop should flash");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftBottom->flash_interval_millis, FLASH_SLOW,"leftBottom should flash fast");
    TEST_ASSERT_EQUAL_MESSAGE(meters.leftTop->flash_interval_millis, FLASH_SLOW,"leftTop should flash fast");    
}
void preTest(){
    current_time_millis = 0;
    gamestate_init(&gameState);
    setupHandlers();
    reset_sounds_for_new_game();
}

void setup() {
    sound_init_for_testing();
    setupHandlers();
    setupMeters();
    delay(1000);
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_INFO, &Serial, true);
    UNITY_BEGIN();

    //simple meter tests
    //preTest();
    //RUN_TEST(test_game_setup);
    preTest();
    RUN_TEST(test_game_time_progression);
    //RUN_TEST(test_one_team_wins_no_ot);
    //RUN_TEST(test_one_team_overtime);

    UNITY_END();

}
void loop() {
    delay(500);
}