#include <Arduino.h>
#include <Teams.h>
#include <Proximity.h>
#include <unity.h>

#define THRESHOLD_SECS 1
ButtonProximity bp = ButtonProximity(THRESHOLD_SECS);

void test_initially_not_close(void){    
    TEST_ASSERT_FALSE(bp.isBluClose() ); 
    TEST_ASSERT_FALSE(bp.isRedClose() );
    TEST_ASSERT_FALSE(bp.isTeamClose(Team::RED));
    TEST_ASSERT_FALSE(bp.isTeamClose(Team::BLU));
}

void test_red_close(void){
    bp.update(true,false);
    TEST_ASSERT_TRUE(bp.isBluClose() ); 
    TEST_ASSERT_FALSE(bp.isRedClose() );
    delay(400);
    TEST_ASSERT_TRUE(bp.isBluClose() ); 
    TEST_ASSERT_FALSE(bp.isRedClose() );
    delay(400);
    TEST_ASSERT_TRUE(bp.isBluClose() ); 
    TEST_ASSERT_FALSE(bp.isRedClose() );
    delay(400);
    TEST_ASSERT_FALSE(bp.isBluClose() ); 
    TEST_ASSERT_FALSE(bp.isRedClose() );    
}

void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_initially_not_close);
    UNITY_END();
}

void loop() {
    delay(500);
}

