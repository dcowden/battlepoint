#include <unity.h>
void test_test(){

};

void RUN_UNITY_TESTS(){
    UNITY_BEGIN();
    RUN_TEST(test_test);
    UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
 
    delay(2000);

    RUN_UNITY_TESTS  ();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}

#else

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif