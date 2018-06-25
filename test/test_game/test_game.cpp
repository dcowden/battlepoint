#include <Arduino.h>
#include <Teams.h>
#include <unity.h>
#include <Game.h>


void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    //RUN_TEST(test_team_color_mappings);
    //RUN_TEST(test_team_chars);
    UNITY_END();
}

void loop() {
    delay(500);
}
