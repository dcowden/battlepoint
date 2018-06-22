#include <Arduino.h>
#include "ControlPoint.h"
#include <unity.h>
#include <Teams.h>

void test_control_point_initial_value(void){
    ControlPoint cp = ControlPoint();
    cp.init(5);
    TEST_ASSERT_FALSE(cp.isCaptured());
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isCapturedBy(Team::RED));
    TEST_ASSERT_FALSE(cp.isCapturedBy(Team::BLU));
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_FALSE(cp.isOn(Team::BLU));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
}

void test_basic_blu_capture(void ){
    ControlPoint cp = ControlPoint();
    TestProximity tp = TestProximity();
    tp.setBluClose(true);
    cp.init(1);
    cp.update(&tp);
    delay(200);
    cp.update(&tp);
    TEST_ASSERT_INT_WITHIN(1,20,cp.getPercentCaptured());
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_FALSE(cp.isCaptured());
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::BLU, cp.getCapturing());
    delay(900);
    cp.update(&tp);
    //counter intuitieve: percent captured is reset to zero after capture
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_TRUE(cp.isCaptured());
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_EQUAL(Team::BLU, cp.getOwner());
    TEST_ASSERT_TRUE(cp.isCapturedBy(Team::BLU));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
}

void test_contested(void){
    ControlPoint cp = ControlPoint();
    TestProximity tp = TestProximity();
    tp.setBluClose(true);
    tp.setRedClose(true);
    cp.init(1);
    cp.update(&tp);
    delay(200);
    cp.update(&tp);
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_TRUE(cp.isOn(Team::RED));
    TEST_ASSERT_FALSE(cp.isCaptured());
    TEST_ASSERT_TRUE(cp.isContested());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_FALSE(cp.isCapturedBy(Team::BLU));
    TEST_ASSERT_FALSE(cp.isCapturedBy(Team::RED));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
}

void test_count_back_down(void){
    ControlPoint cp = ControlPoint();
    TestProximity tp = TestProximity();
    tp.setBluClose(true);
    cp.init(1);
    cp.update(&tp);
    delay(200);
    cp.update(&tp);
    TEST_ASSERT_INT_WITHIN(1,20, cp.getPercentCaptured());
    tp.setBluClose(false);
    delay(100);
    cp.update(&tp);
    TEST_ASSERT_EQUAL(10, cp.getPercentCaptured());
    delay(400);
    cp.update(&tp);
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_FALSE(cp.isOn(Team::BLU));
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_FALSE(cp.isCaptured());
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_FALSE(cp.isCapturedBy(Team::BLU));
    TEST_ASSERT_FALSE(cp.isCapturedBy(Team::RED));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
}

void test_capture_disabled(void){
    ControlPoint cp = ControlPoint();
    cp.setBluCaptureEnabled(false);
    cp.setRedCaptureEnabled(true);
    TestProximity tp = TestProximity();
    tp.setBluClose(true);
    cp.init(1);
    cp.update(&tp);
    delay(200);
    cp.update(&tp);

    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_FALSE(cp.isCaptured());
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));   
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());     
}

void setup() {
    
    delay(2000);
    Serial.begin(115200);
    UNITY_BEGIN();
    RUN_TEST(test_control_point_initial_value);
    RUN_TEST(test_basic_blu_capture);
    RUN_TEST(test_contested);
    RUN_TEST(test_count_back_down);
    RUN_TEST(test_capture_disabled);
    UNITY_END();

}
void loop() {
    delay(500);
}