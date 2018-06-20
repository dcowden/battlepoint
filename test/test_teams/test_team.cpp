#include <Arduino.h>
#include <Teams.h>
#include <FastLED.h>
#include <unity.h>

void test_team_color_mappings(void){
    CRGB blue = CRGB::Blue;
    CRGB red = CRGB::Red;
    CRGB black = CRGB::Black;
    CRGB aqua = CRGB::Aqua;
    TEST_ASSERT_EQUAL(blue,getTeamColor(Team::BLU) );   
    TEST_ASSERT_EQUAL(red,getTeamColor(Team::RED) );
    TEST_ASSERT_EQUAL(black,getTeamColor(Team::NOBODY) );
    TEST_ASSERT_EQUAL(aqua,getTeamColor(Team::ALL) );
}
void test_team_chars(void){
    TEST_ASSERT_EQUAL('B',teamTextChar(Team::BLU) ); 
    TEST_ASSERT_EQUAL('R',teamTextChar(Team::RED) );  
    TEST_ASSERT_EQUAL('-',teamTextChar(Team::NOBODY) ); 
    TEST_ASSERT_EQUAL('+',teamTextChar(Team::ALL) );  
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

