 #include "battlepoint.h"

/**
 * Tests for the meters. 
 * Wire up both strands. Two meters shouild be reversed. All should count from blank to fully lit, one click per second
 */

 void test_meters_setup(LedMeter* captureMeter, LedMeter* ownerMeter, LedMeter* timer1Meter, LedMeter* timer2Meter);
 void test_meters_loop(LedMeter* captureMeter, LedMeter* ownerMeter, LedMeter* timer1Meter, LedMeter* timer2Meter);

 /**
  * Tests for proximity. Start the test and click the buttons, watching
  * the serial console. proximity output should track correctly with who is on the point
  */
 void test_buttons_setup(Proximity* proximity);
 void test_buttons_loop(Proximity* proximity);

 /**
  * Tests for the control point.
  * Run the test, and watch the console while pressing the switches. The 
  * Control point should act correctly
  */
 void test_controlpoint_setup(ControlPoint* cp, LedMeter* captureMeter, LedMeter* ownerMeter,LedMeter* timer1Meter, LedMeter* timer2Meter, int captureSeconds);
 void test_controlpoint_loop(ControlPoint* cp, Proximity* prox);
 
