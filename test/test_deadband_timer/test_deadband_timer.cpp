#include <Arduino.h>
#include <Teams.h>
#include <CooldownTimer.h>
#include <unity.h>


class TestNewTimer {
    public:
       TestNewTimer(long interval){
           _timer = new CooldownTimer(interval);

       }
       boolean okToRun(){
           return _timer->canRun();
       }
    private:
       CooldownTimer* _timer;
};

void test_basic_timer(void){
    CooldownTimer ct = CooldownTimer(500);
    TEST_ASSERT_TRUE ( ct.canRun() ) ;
    TEST_ASSERT_FALSE ( ct.canRun() );
    delay(100);
    TEST_ASSERT_FALSE ( ct.canRun() );
    delay(600);
    TEST_ASSERT_TRUE ( ct.canRun()) ;
    delay(600);
    TEST_ASSERT_TRUE ( ct.canRun()) ;    
}

void test_timer_using_class(void){
    TestNewTimer tt = TestNewTimer(500);
    TEST_ASSERT_TRUE ( tt.okToRun() );
    TEST_ASSERT_FALSE ( tt.okToRun() );
    delay(600);
    TEST_ASSERT_TRUE ( tt.okToRun() );
}
void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_basic_timer);
    RUN_TEST(test_timer_using_class);
    UNITY_END();
}

void loop() {
    delay(500);
}
