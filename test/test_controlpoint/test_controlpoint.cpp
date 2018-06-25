#include <Arduino.h>
#include "ControlPoint.h"
#include <unity.h>
#include <Teams.h>

void test_control_point_initial_value(void){
    TestProximity tp = TestProximity();
    ControlPoint cp = ControlPoint(&tp);
    cp.init(5);
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_FALSE(cp.isOn(Team::BLU));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
}

void test_basic_blu_capture(void ){
    TestProximity tp = TestProximity();
    ControlPoint cp = ControlPoint(&tp);
    tp.setBluClose(true);
 
    cp.init(1);
    cp.update();
    delay(200);
    cp.update();
    TEST_ASSERT_INT_WITHIN(1,20,cp.getPercentCaptured());
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));    
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::BLU, cp.getCapturing());
    delay(900);
    cp.update();
    //counter intuitieve: percent captured is reset to zero after capture
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_EQUAL(Team::BLU, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
}

void test_contested(void){
    TestProximity tp = TestProximity();
    ControlPoint cp = ControlPoint(&tp);
    tp.setBluClose(true);
    tp.setRedClose(true);
    cp.init(1);
    cp.update();
    delay(200);
    cp.update();
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_TRUE(cp.isOn(Team::BLU));
    TEST_ASSERT_TRUE(cp.isOn(Team::RED));
    TEST_ASSERT_TRUE(cp.isContested());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
}

void test_count_back_down(void){
    TestProximity tp = TestProximity();
    ControlPoint cp = ControlPoint(&tp);
    cp.init(1);    
    tp.setBluClose(true);

    cp.update();
    delay(200);
    cp.update();
    TEST_ASSERT_INT_WITHIN(1,20, cp.getPercentCaptured());
    tp.setBluClose(false);
    delay(100);
    cp.update();
    TEST_ASSERT_EQUAL(10, cp.getPercentCaptured());
    delay(400);
    cp.update();
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());
    TEST_ASSERT_FALSE(cp.isOn(Team::BLU));
    TEST_ASSERT_FALSE(cp.isOn(Team::RED));
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getOwner());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing());
}

void test_capture_disabled(void){
    TestProximity tp = TestProximity();
    tp.setRedClose(true);

    ControlPoint cp = ControlPoint(&tp);
    cp.init(1);
    cp.setBluCaptureEnabled(false);
    cp.setRedCaptureEnabled(false);
    TEST_ASSERT_FALSE(cp.getBluCaptureEnabled());
    TEST_ASSERT_FALSE(cp.getRedCaptureEnabled());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing()); 
    cp.update();
    delay(200);
    cp.update();

    TEST_ASSERT_TRUE(cp.isOn(Team::RED));
    TEST_ASSERT_FALSE(cp.isOn(Team::BLU));
    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_EQUAL(Team::NOBODY, cp.getCapturing()); 
    TEST_ASSERT_EQUAL(0, cp.getPercentCaptured());        
}

void test_reverse_capture(void){
    TestProximity tp = TestProximity();
    ControlPoint cp = ControlPoint(&tp);
    cp.init(1);
    cp.setBluCaptureEnabled(true);
    cp.setRedCaptureEnabled(true);
    
    tp.setRedClose(true);
    cp.setOwner(Team::BLU);    
    cp.update();
    delay(200);
    cp.update();
    TEST_ASSERT_INT_WITHIN(1,20, cp.getPercentCaptured()); 
    TEST_ASSERT_TRUE(cp.isOn(Team::RED));
    TEST_ASSERT_EQUAL(Team::BLU, cp.getOwner());

    TEST_ASSERT_FALSE(cp.isContested());
    TEST_ASSERT_EQUAL(Team::RED, cp.getCapturing()); 
    tp.setBluClose(true);
    cp.update();
    delay(100);
    cp.update();
    //no decrement or increment if contested
    TEST_ASSERT_TRUE(cp.isContested());
    TEST_ASSERT_INT_WITHIN(1,20, cp.getPercentCaptured());
    tp.setRedClose(false);
    cp.update();
    delay(100);
    cp.update();
    TEST_ASSERT_INT_WITHIN(1,10, cp.getPercentCaptured());
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
    RUN_TEST(test_reverse_capture);
    UNITY_END();

}
void loop() {
    delay(500);
}