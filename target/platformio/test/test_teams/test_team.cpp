#include <Arduino.h>
#include <Teams.h>
#include <unity.h>

void test_team_color_mappings(void){
    TeamColor blue = TeamColor::COLOR_BLUE;
    TeamColor red = TeamColor::COLOR_RED;
    TeamColor black = TeamColor::COLOR_BLACK;
    TeamColor aqua = TeamColor::COLOR_AQUA;
    TEST_ASSERT_EQUAL(blue,getTeamColor(Team::BLU) );   
    TEST_ASSERT_EQUAL(red,getTeamColor(Team::RED) );
    TEST_ASSERT_EQUAL(black,getTeamColor(Team::NOBODY) );
    TEST_ASSERT_EQUAL(aqua,getTeamColor(Team::ALL) );
}
void test_team_chars(void){
    TEST_ASSERT_EQUAL_STRING("BLU",teamTextChar(Team::BLU) ); 
    TEST_ASSERT_EQUAL_STRING("RED",teamTextChar(Team::RED) );  
    TEST_ASSERT_EQUAL_STRING("NONE",teamTextChar(Team::NOBODY) ); 
    TEST_ASSERT_EQUAL_STRING("ALL",teamTextChar(Team::ALL) );  
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_team_color_mappings);
    RUN_TEST(test_team_chars);
    UNITY_END();
}

void loop() {
    delay(500);
}

