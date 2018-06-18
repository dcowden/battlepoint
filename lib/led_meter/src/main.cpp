#include <Arduino.h>
#define FASTLED_ESP32_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define INTERRUPT_THRESHOLD 1
#include <FastLED.h>
#include "LedMeter.h"

#define LED_PIN 4
#define LED_CNT 19
#define FASTLED_SHOW_CORE 0

CRGB leds[LED_CNT];

// pins 6-11 not usable
// pins 34-39 input only without pullups
// pins 0 and 2 have to have right level on startup
// pins 1 and 3 are TX0 and RX0, used for serial monitor
// pins 22 and 21 are I2C SCL and SDA
// pins 9 and 10 are RX1 and TX1
// vspi is on 23,19,18, and 5
// spi is on 15,14,12, and 13
// that leaves these left:
// 4, 16,17,27,26,25
//LedRange testRange1 [2] = {  { 0, 4 },{5,10} } ; 
LedRange testRange2 [2] = {  { 0, 9 },{18,10} } ;

static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;
/** show() for ESP32
    Call this function instead of FastLED.show(). It signals core 0 to issue a show,
    then waits for a notification that it is done.
*/
void FastLEDshowESP32()
{
  if (userTaskHandle == 0) {
    // -- Store the handle of the current task, so that the show task can
    //    notify it when it's done
    userTaskHandle = xTaskGetCurrentTaskHandle();

    // -- Trigger the show task
    xTaskNotifyGive(FastLEDshowTaskHandle);

    // -- Wait to be notified that it's done
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
    ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    userTaskHandle = 0;
  }
}

/** show Task
    This function runs on core 0 and just waits for requests to call FastLED.show()
*/
void FastLEDshowTask(void *pvParameters)
{
  // -- Run forever...
  for (;;) {
    // -- Wait for the trigger
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // -- Do the show (synchronously)
    FastLED.show();

    // -- Notify the calling task
    xTaskNotifyGive(userTaskHandle);
  }
}

//LedMeter t1 = LedMeter(leds,testRange1,2);
LedMeter t2 = LedMeter(leds,testRange2,2,CRGB::Blue,CRGB::Black);


void setup() {
    Serial.begin(115200);
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    //t1.setMaxValue(100);
    //t1.setBgColor(CRGB::Red);
    //t1.setBgColor(CRGB::Blue);
 
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_CNT);
    FastLED.setBrightness(40);

    int core = xPortGetCoreID();


    // -- Create the FastLED show task
    xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, &FastLEDshowTaskHandle, FASTLED_SHOW_CORE);
        


    Serial.print("Main code running on core ");
    Serial.println(core);
}

void loop() {

    for(int i=0;i<100;i++){
        //Serial.print("loop=");
        //Serial.print(i);
        t2.setValue(i);
        //t1.setValue(i);
        //FastLED.show();
        FastLED.delay(100);
    }
}