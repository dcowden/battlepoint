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
#include <NonBlockingRtttl.h>


//hardware
#define I2C_ADDRESS 0x3C   // 0X3C+SA0 - 0x3C or 0x3D
#define SIDE_LED_SIZE 8
#define RIGHT_LED_PIN 12
#define LEFT_LED_PIN 14
#define BIG_HIT_PIN 26  //32
#define LITTLE_HIT_PIN 27   //33
#define TOUCH_DELTA_THRESHOLD 10
#define BUZZER_PIN 25
#define MENU_FONT X11fixed7x14


//app settings
#define BATTLEPOINT_VERSION "1.1.3"
#define AFTER_CARD_READ_DELAY_MS 1000
#define CARD_BUFFER_BYTES 500
#define TAG_PRESENT_TIMEOUT 100
#define LED_BRIGHTNESS 100
#define CARDREAD_CONFIRM_NUM_BLINKS 6
#define CARDREAD_CONFIRM_BLINK_MS 500
#define HIT_CONFIRM_NUM_BLINKS 5
#define HIT_CONFIRM_BLINK_MS 200


//https://github.com/neverfa11ing/FlipperMusicRTTTL/tree/main/RTTTL_generics
const char * mario = "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
const char * SOUND_LIFE1 =  "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * LITTLE_HIT = "bighit:d=8,o=4,b=450:e.5,e.5,e.5";
const char * BIG_HIT = "little:d=8,o=4,b=450:8g,8g,8g";
const char * DEAD_MUSIC = "dead:d=32,o=4,b=64:32g,32p,32g,32p,32g,32p,2d#";

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
LifeConfig currentConfig;

//TODO: smells like need a controller for ui. 
//ideally, with ui events
bool notifiedDead = false;


void blinkLedsWithColor(int numBlinks, CRGB color, int blink_rate_millis);
void updateDisplay();
void updateLEDs();

void updateUI(){
    if (! notifiedDead ){
        if ( currentConfig.state == LifeStage::DEAD){
            rtttl::begin(BUZZER_PIN,DEAD_MUSIC);
            notifiedDead = true;
        }
    }
    rtttl::play();
    updateDisplay();
    updateLEDs();
    FastLED.delay(10);
}

void uiDelay(int millis_to_wait ){
  //delay while updating sound, leds, and display
  long s = millis();
  long end = millis_to_wait + s;
  while ( millis() < end){
      updateUI();
  }
}

void bigHitButtonClick(Button2& b){
  if ( currentConfig.state == LifeStage::ALIVE ){  
    Log.traceln("LittleHit");
    little_hit(currentConfig, millis());
    rtttl::begin(BUZZER_PIN, LITTLE_HIT);
    start_invuln(currentConfig, millis());
  }
}

void littleHitButtonClick(Button2& b){
  if ( currentConfig.state == LifeStage::ALIVE ){
    Log.traceln("BigHit");
    big_hit(currentConfig,millis() );
    rtttl::begin(BUZZER_PIN, BIG_HIT);
    start_invuln(currentConfig, millis());
  }  
}

void setupLeds(){
  FastLED.addLeds<NEOPIXEL, LEFT_LED_PIN  >(leftLeds, SIDE_LED_SIZE);  
  FastLED.addLeds<NEOPIXEL, RIGHT_LED_PIN  >(rightLeds, SIDE_LED_SIZE);  
}

void setupButtons(){
    pinMode(BIG_HIT_PIN,INPUT_PULLUP);
    pinMode(LITTLE_HIT_PIN,INPUT_PULLUP);
    bigHitButton.begin(BIG_HIT_PIN);
    littleHitButton.begin(LITTLE_HIT_PIN);
    bigHitButton.setTapHandler(bigHitButtonClick);
    littleHitButton.setTapHandler(littleHitButtonClick);    
}

void setupMeters(){
  FastLED.setBrightness(LED_BRIGHTNESS);
  int IDX=SIDE_LED_SIZE-1;
  initMeter(&leftMeter,"left",leftLeds,IDX,0);
  initMeter(&rightMeter,"right",rightLeds,IDX,0);
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

void updateMeters(CRGB fgColor, int val, long blink_rate_millis ){
    //TODO: need a composite meter to avoid this kind of stuff    
    setMeterValue(&leftMeter,val,fgColor,blink_rate_millis); 
    setMeterValue(&rightMeter,val,fgColor,blink_rate_millis); 
    updateLedMeter(&leftMeter,millis());
    updateLedMeter(&rightMeter,millis()); 
    FastLED.show();     
}
void updateMeters (CRGB fgColor, int val ){
  updateMeters(fgColor,val,FlashInterval::FLASH_NONE);
}

 
void blinkLedsWithColor(int numBlinks, CRGB color, int blink_rate_millis){
  long delay_ms = numBlinks* blink_rate_millis;
  updateMeters(color,leftMeter.max_val,blink_rate_millis);
  uiDelay(delay_ms);
} 


void POST(){
   Log.noticeln("POST...");
   blinkLedsWithColor(6, CRGB::Blue, 500 );
   Log.noticeln("POST COMPLETE");   
}

void setup(void) 
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.warning("Starting...");  
  pinMode(BUZZER_PIN, OUTPUT);
  rtttl::begin(BUZZER_PIN, SOUND_LIFE1);
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
  rtttl::play();
}

void updateDisplay(){  
  char SPACE = ' ';
  oled.setCursor(0,0);
  oled.print("HP: ");  oled.print(currentConfig.hp);  oled.print("/"); oled.print(currentConfig.max_hp); 
  oled.clearToEOL(); oled.println("");
  oled.clearToEOL(); oled.println("");
  oled.print("DMG: ");  oled.print(currentConfig.little_hit); oled.print("/"); oled.print(currentConfig.big_hit); 
  oled.clearToEOL(); oled.println("");
  oled.print("STS: "); oled.print(lifeStatus(currentConfig));
  oled.clearToEOL(); oled.println("");
}  

long getRespawnFlashInterval(LifeConfig &config){
    // this will flash closer as we near respawn completion
    //30000 ms left = 1.5 sec flash
    //10000 ms left 0.5 sec flash
    //5000 ms left 0.25 sec flash
    int MULTIPLIER =20;
    return config.respawn_ms / MULTIPLIER;
}

void updateLEDs(){ 
    CRGB fgColor = CRGB::Black;
    long flash_interval = FlashInterval::FLASH_NONE;
    int meterVal = 0;

    if ( currentConfig.state == LifeStage::ALIVE){
      fgColor = CRGB::Green;
      flash_interval = FlashInterval::FLASH_NONE;
      meterVal = currentConfig.hp;
    }
    else if ( currentConfig.state == LifeStage::DEAD){
      fgColor = CRGB::Red;
      flash_interval = FlashInterval::FLASH_SLOW;
      meterVal = currentConfig.max_hp;
    }
    else if ( currentConfig.state == LifeStage::INVULNERABLE){
      fgColor = CRGB::Yellow;
      flash_interval = FlashInterval::FLASH_SLOW;
      meterVal = currentConfig.max_hp;
    }
    else if ( currentConfig.state == LifeStage::RESPAWNING){
      fgColor = CRGB::Orange;
      flash_interval = getRespawnFlashInterval(currentConfig);
      meterVal = currentConfig.max_hp;
    }
    else {
      //unknown state
      fgColor = CRGB::Black;
      flash_interval = FlashInterval::FLASH_NONE;
      meterVal = 0;      
    }

    setMeterValues(&leftMeter,currentConfig.max_hp, meterVal, fgColor,  flash_interval);
    setMeterValues(&rightMeter,currentConfig.max_hp, meterVal, fgColor,  flash_interval);    

    long m = millis();
    updateLedMeter(&leftMeter, m );
    updateLedMeter(&rightMeter, m);

}




void handleConfigCard(){
    Log.warningln("Respawn Card");
    
    int max_hp = doc["max_hp"];
    if ( max_hp){
      currentConfig.max_hp = doc["max_hp"];
    }

    int hp = doc["hp"];
    if ( hp){
      currentConfig.hp = hp;
    }
    else{
      currentConfig.hp = currentConfig.max_hp;
    }
  
    int big_hit = doc["big_hit"];
    if ( big_hit){
      currentConfig.big_hit = big_hit;
    }

    int little_hit = doc["little_hit"];
    if ( little_hit){
      currentConfig.little_hit = little_hit;
    }

    int invuln_ms = doc["invuln_ms"];
    if ( invuln_ms){
      currentConfig.invuln_ms = invuln_ms;
    }

    int respawn_ms = doc["respawn_ms"];
    if ( respawn_ms){
      currentConfig.respawn_ms = respawn_ms;
    }

    Log.traceln("Config: hp=%d, max_hp=%d, big_hit=%d, little_hit=%d, respawn_ms=%l, respawn_ms=%l",
       hp, max_hp, big_hit, little_hit, invuln_ms, respawn_ms );
    requestRespawn(currentConfig, millis() );             
}

void handleRespawnCard(){
  Log.warningln("Requesting Respawn for existing Class");
  requestRespawn(currentConfig, millis() );
}

void handleMedicCard(){
    int hp_to_add = doc["add_hp"];
    if ( hp_to_add ){
      Log.warningln("Adding %d hp.", hp_to_add);
      medic(currentConfig, hp_to_add, millis() );
    }
    else{
      Log.warning("No HP found");
    }
}

void handleFlagCard(){
    Log.warning("picked up flag");
}

void handlePresentedCard(){

  int card_type = doc["card_type"];
  if ( ! card_type ){
    Log.warning("Cannot detected Card Type.");
    return;
  }

  Log.traceln("Card type: %s", card_type); 
  if ( card_type == NFCCardType::CONFIG){
    handleConfigCard();
  }
  else if ( card_type == NFCCardType::FLAG ){
    handleFlagCard();
  }
  else if ( card_type == NFCCardType::RESPAWN){
    handleRespawnCard();
  }
  else if ( card_type == NFCCardType::MEDIC){
    handleMedicCard();
  }
  blinkLedsWithColor(CARDREAD_CONFIRM_NUM_BLINKS, CRGB::Blue, CARDREAD_CONFIRM_BLINK_MS );

  Log.warning("Handle Card Complete...");
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

            if (error) {
                Log.errorln("deserializeJson() failed: ");
                Log.errorln(error.f_str());
            }   
            else{
                handlePresentedCard();
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
    updateLifeStatus(currentConfig, millis() );
    updateUI();

}