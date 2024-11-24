#include <Wire.h>
#include <PN532_HSU.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <FastLED.h>
#include <Ndef.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <ArduinoJson.h>
#include <LedMeter.h>
#include <ArduinoLog.h>
#include <TeamNFC.h>
#include "Button2.h"


#define I2C_ADDRESS 0x3C   // 0X3C+SA0 - 0x3C or 0x3D


#define BATTLEPOINT_VERSION "1.1.3"
#define SIDE_LED_SIZE 10
#define RIGHT_LED_PIN 12
#define LEFT_LED_PIN 14
#define BIG_HIT_PIN 32
#define LITTLE_HIT_PIN 33
#define AFTER_CARD_READ_DELAY_MS 1000
#define CARD_BUFFER_BYTES 500
#define TAG_PRESENT_TIMEOUT 100

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI

#define MENU_FONT X11fixed7x14

CRGB leftLeds[SIDE_LED_SIZE];
CRGB rightLeds[SIDE_LED_SIZE];
LedMeter leftMeter;
LedMeter rightMeter;
SSD1306AsciiWire oled;
Button2 bigHitButton;
Button2 littleHitButton;
HardwareSerial pn532_serial(2);
PN532_HSU pn532_s (pn532_serial);
NfcAdapter nfc = NfcAdapter(pn532_s);
byte nuidPICC[4];
String tagId= "None";
JsonDocument doc;
LifeConfig config;

void bigHitButtonClick(Button2& btn){
  Log.noticeln("BigHit");
  big_hit(&config);
  updateLife(&config);
}

void littleHitButtonClick(Button2& btn){
  Log.noticeln("LittleHit");
  little_hit(&config);
  updateLife(&config);
}

void setupLeds(){
  FastLED.addLeds<NEOPIXEL, LEFT_LED_PIN  >(leftLeds, SIDE_LED_SIZE);  
  FastLED.addLeds<NEOPIXEL, RIGHT_LED_PIN  >(rightLeds, SIDE_LED_SIZE);  
}

void setupButtons(){
    bigHitButton.begin(BIG_HIT_PIN);
    littleHitButton.begin(LITTLE_HIT_PIN);
    bigHitButton.setTapHandler(bigHitButtonClick);
    littleHitButton.setTapHandler(littleHitButtonClick);    
}

void setupMeters(){
  initMeter(&leftMeter,"left",leftLeds,0,SIDE_LED_SIZE-1);
  initMeter(&rightMeter,"right",rightLeds,0,SIDE_LED_SIZE-1);
}

void setupOLED(){
  Wire.setClock(400000L);  
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(MENU_FONT);
  oled.displayRemap(false);
  oled.clear();
  oled.println("BattlePoint v1.1"); 
  oled.println("HitTracker..."); 
}

void updateMeters(CRGB fgColor, int val ){
    setMeterValue(&leftMeter,val,fgColor); 
    setMeterValue(&rightMeter,val,fgColor); 
    updateLedMeter(&leftMeter,millis());
    updateLedMeter(&rightMeter,millis()); 
    FastLED.show();     
}


void blinkLedsWithColor(int numBlinks, CRGB color, int blink_rate_millis){

  int val = SIDE_LED_SIZE;
  for ( int i=0;i<numBlinks;i++){
    updateMeters(color, val);
    if ( val == 0 ){
       val = SIDE_LED_SIZE;
    }        
    else{
      val = 0;
    }
    FastLED.delay(blink_rate_millis);
  }
}


void POST(){
   Log.noticeln("POST...");
   blinkLedsWithColor(6, CRGB::Blue, 500 );
   Log.noticeln("POST COMPLETE");   
}

void updateDisplay(){  
  char SPACE = ' ';
  oled.setCursor(0,0);
  oled.print("HP: ");  oled.print(config.hp);  oled.print("/"); oled.print(config.max_hp); 
  oled.clearToEOL(); oled.println("");
  oled.print("Lives: "); oled.print(config.spawns); oled.print("/"); oled.print(config.max_spawns);
  oled.clearToEOL(); oled.println("");
  oled.print("DMG: ");  oled.print(config.little_hit); oled.print("/"); oled.print(config.big_hit); 
  oled.clearToEOL(); oled.println("");
  oled.print("STS: ");
  if ( config.is_dead ){
    oled.print("DEAD");
  }
  else if ( config.is_invul){
    oled.print("INVULN");
  } 
  else{
    oled.print("OK");
  }
  oled.clearToEOL(); oled.println("");
}  

const char* lifeStatusText(LifeConfig* config){
  if ( config->is_dead ){
    return "DEAD";
  }
  else if ( config->is_invul){
    return "INVULN";
  } 
  else{
    return "OK";
  }  
}
void printLifeConfig(){
  Log.noticeln("HP: %d/%d", config.hp,config.max_hp);
  Log.noticeln("Lives: %d/%d", config.spawns,config.max_spawns);
  Log.noticeln("DMG: %d/%d", config.little_hit,config.big_hit);
  Log.noticeln("STS: %s", lifeStatusText(&config));
}

void updateLEDs(){
  CRGB fg = CRGB::Green;
  long flashing = FlashInterval::FLASH_NONE;
  int meter_val = 0;
  if ( config.is_dead ){
    fg = CRGB::Red;
    flashing = FlashInterval::FLASH_SLOW;
    meter_val = config.max_hp;
  }
  else{
    fg = CRGB::Green;
    flashing = FlashInterval::FLASH_NONE;
    meter_val = config.hp;
  }
  configureMeter(&leftMeter,config.max_hp, meter_val, fg, CRGB::Black);
  configureMeter(&rightMeter,config.max_hp, meter_val, fg, CRGB::Black);  
  leftMeter.flash_interval_millis = flashing;
  rightMeter.flash_interval_millis = flashing;
  //interesting: why do they flash out of sync?
  long m = millis();
  updateLedMeter(&leftMeter, m );
  updateLedMeter(&rightMeter, m);

}

void setup(void) 
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.warning("Starting...");  
  Wire.begin();
  Wire.setClock(100000);
  Log.noticeln("WIRE [OK]");    
  nfc.begin();
  Log.noticeln("NFC  [OK]");    
  setupOLED();
  Log.noticeln("OLED [OK]");     
  setupLeds();
  setupMeters();
  Log.noticeln("LEDS [OK]");  
  setupButtons();
  Log.noticeln("BTNS [OK]");
  POST();  
  Log.noticeln("DONE [OK]");  
}

void reconfigure(){

  Log.notice("Before ReConfig: ");   printLifeConfig();

  int card_type = doc["card_type"];
  if ( ! card_type ){
    Log.warning("Cannot detected Card Type.");
    return;
  }

  //Log.warning("Card type: "); Log.warningln(card_type);
  if ( card_type == NFCCardType::CONFIG){
    Log.warningln("Reconfiguring");

    int max_spawns = doc["max_spawns"];
    if ( max_spawns ){
      Serial.print("max_spawns=");Serial.print(max_spawns);
      config.max_spawns = max_spawns;
    }

    int max_hp = doc["max_hp"];
    if ( max_hp){
      Serial.print(" max_hp=");Serial.print(max_hp);
      config.max_hp = doc["max_hp"];
    }
    int hp = doc["hp"];
    if ( hp){
      Serial.print(" hp=");Serial.print(hp);
      config.hp = hp;
    }
    else{
      config.hp = config.max_hp;
    }
  
    int big_hit = doc["big_hit"];
    if ( big_hit){
      Serial.print(" big_hit=");Serial.print(big_hit);
      config.big_hit = big_hit;
    }

    int little_hit = doc["little_hit"];
    if ( little_hit){
      Serial.print(" little_hit=");Serial.print(little_hit);
      config.little_hit = little_hit;
    }

    int invuln_ms = doc["invuln_ms"];
    if ( invuln_ms){
      Serial.print(" invuln_ms=");Serial.print(invuln_ms);
      config.invuln_ms = invuln_ms;
    }
    config.is_dead = false;
    config.is_invul = false;
                   
  }
  else if ( card_type == NFCCardType::FLAG ){
    Log.warning("picked up flag");
  }
  else if ( card_type == NFCCardType::RESPAWN){
    Log.warningln("Respawning..");
    respawn(&config);
  }
  else if ( card_type == NFCCardType::MEDIC){
    int hp_to_add = doc["add_hp"];
    if ( hp_to_add ){
       //Log.warning("Adding HP: ");Log.warningln(hp_to_add);
       medic(&config, hp_to_add);
    }
    else{
      Log.warning("No HP found");
    }

  }
  blinkLedsWithColor(6, CRGB::Blue, 200 );
  updateLife(&config);
  Log.notice("Before ReConfig: ");   printLifeConfig();
  Log.warning("Reconfiguring DONE...");
}

 
void readNFC() 
{
 
 if (nfc.tagPresent(TAG_PRESENT_TIMEOUT))
 {
    NfcTag tag = nfc.read();
    tag.print();
    tagId = tag.getUidString();

    delay(AFTER_CARD_READ_DELAY_MS);
    
    if ( tag.hasNdefMessage() ){
        NdefMessage m = tag.getNdefMessage();
        if ( m.getRecordCount() > 0 ){
            NdefRecord r = m.getRecord(0);
            byte payload_buffer[CARD_BUFFER_BYTES];
            r.getPayload(payload_buffer);
            char* as_chars = (char*)payload_buffer;

            as_chars+=(3*sizeof(char)); //record has 'en in front of it, need to get rid of that.
            Log.traceln("Got Payload");Log.traceln(as_chars);

            
            DeserializationError error = deserializeJson(doc, as_chars);

            // Test if parsing succeeds
            if (error) {
                Log.errorln("deserializeJson() failed: ");
                Log.errorln(error.f_str());
            }   
            else{
              reconfigure();
            }

            
        } 
    
    }
 }
}

void loop() 
{
    bigHitButton.loop();
    littleHitButton.loop();  
    readNFC();
    updateLEDs();
    updateDisplay();
    FastLED.show();
}