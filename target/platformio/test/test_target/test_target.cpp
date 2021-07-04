#include <Arduino.h>
#include <target.h>
#include <Clock.h>
#include <unity.h>
#define BP_DEBUG 1

#define MOCK_DATA_LENGTH 129


int mockData[MOCK_DATA_LENGTH] = {
    3000,
    118, 12, 0, 0, 6, 117
    ,980 ,1023 ,1023,1023 ,1023 ,703 ,319 ,116 ,12 ,0 ,0 ,0 ,0,0 ,0
    ,0 ,0 ,0 ,0 ,0 ,0 ,1 ,13 ,37 ,336 ,279 ,131 ,0 ,0 ,0 ,0 ,0 ,0 ,0
    ,0 ,0 ,0 ,229 ,79 ,13 ,0 ,0 ,0 ,0 ,0 ,16 ,0 ,0 ,266 ,281 ,154 ,103 ,98
    ,859 ,1010 ,962 ,1023 ,767 ,773 ,404 ,167 ,27 ,0 ,0 ,0 ,0 ,0 ,3 ,20 ,0
    ,6 ,402 ,514 ,650 ,835 ,930 ,1023 ,1023 ,1023 ,616 ,336 ,159 ,122 ,298
    ,488 ,228 ,100 ,28 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    ,135,113,267
};
int sample_number = 0;

int mock_adc_reader(){
    int v = mockData[sample_number++]; 
    return v;
}

void test_simple_target(void){
    Serial.println("Testing Target Code");
    TestClock tc = TestClock();
    tc.setTime(123456);
    TargetSettings t;
    //t.pin=0;
    //t.total_hits=0;
    t.last_hit_millis=0;
    t.trigger_threshold=2000;
    t.hit_energy_threshold=10000.0;

    TargetHitScanResult r = check_target(mock_adc_reader, t, &tc);
    TEST_ASSERT_FLOAT_WITHIN(14249.97, r.last_hit_energy , 0.01 );
    TEST_ASSERT_FLOAT_WITHIN(249.083582, r.peak_frequency , 0.01 );
    TEST_ASSERT_TRUE_MESSAGE(r.hit_millis == tc.milliseconds(), "Last hit milli  expected now");
    TEST_ASSERT_TRUE_MESSAGE(r.was_sampled == 1, "should have been sampled");
    TEST_ASSERT_TRUE_MESSAGE(r.was_hit == 1, "Expected a hit");

}


void setup() {
    delay(1000);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_simple_target);
    UNITY_END();
}

void loop() {
    delay(500);
}
