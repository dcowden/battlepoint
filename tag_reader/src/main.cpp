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


#define I2C_ADDRESS 0x3C   // 0X3C+SA0 - 0x3C or 0x3D


#define BATTLEPOINT_VERSION "1.1.3"
#define SIDE_LED_SIZE 7
#define RIGHT_LED_PIN 12
#define LEFT_LED_PIN 14
#define BIG_HIT_PIN 26  //32
#define LITTLE_HIT_PIN 27   //33
#define TOUCH_DELTA_THRESHOLD 10
#define BUZZER_PIN 25
#define AFTER_CARD_READ_DELAY_MS 200
#define CARD_BUFFER_BYTES 500
#define TAG_PRESENT_TIMEOUT 50
#define LED_BRIGHTNESS 100
#define INVULN_BLINK_RATE 200
#define NUM_INVULN_BLINKS 5
#define DISPLAY_UPDATE_INTERVAL_MS 500

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI

#define MENU_FONT X11fixed7x14

//https://github.com/neverfa11ing/FlipperMusicRTTTL/tree/main/RTTTL_generics
const char * mario = "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
const char * SOUND_LIFE1 =  "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * LITTLE_HIT = "bighit:d=8,o=4,b=450:e.5,e.5,e.5";
const char * BIG_HIT = "little:d=8,o=4,b=450:8g,8g,8g";
const char * CARD_OK_SCAN = "scan:d=8,o=4,b=450:32p.,d.6,32p.,g.6,32p.";
const char * CARD_NO_SCAN = "scan:d=8,o=4,b=450:2d#";
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
String lastTagId = "None";
JsonDocument doc;
LifeConfig config;


bool notifiedDead = false;
bool notifiedAlive = false;
long last_display_update = 0;

void blinkLedsWithColor(int numBlinks, CRGB color, int blink_rate_millis);
void delayWithSound(int millis_to_wait){
  long s = millis();
  long end = millis_to_wait + s;
  while ( millis() < end){
    rtttl::play();
    FastLED.delay(10);
  }
}

int getNumInvulnBlinks(){
   return (int)(config.invuln_ms / INVULN_BLINK_RATE);
}

void doLittleClick(){
  if ( config.state == LifeStage::ALIVE){  
    //Log.noticeln("LittleHit");
    bool isStillAlive = little_hit(config);
    //updateLife(config,millis());
    rtttl::begin(BUZZER_PIN, LITTLE_HIT);
    if ( isStillAlive ){
      blinkLedsWithColor(getNumInvulnBlinks(), CRGB::Yellow, INVULN_BLINK_RATE);
    }
  }
}
void doBigClick(){
  if ( config.state == LifeStage::ALIVE ){
    //Log.noticeln("BigHit");
    bool isStillAlive =  big_hit(config);
    //updateLife(config,milis());
    rtttl::begin(BUZZER_PIN, BIG_HIT);
    if ( isStillAlive ){
      blinkLedsWithColor(getNumInvulnBlinks(), CRGB::Red, INVULN_BLINK_RATE);
    }
    
  }  
}
void bigHitButtonClick(Button2& b){
  doBigClick();
}

void littleHitButtonClick(Button2& b){
   doLittleClick();
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

void updateMeters(CRGB fgColor, int val ){
    setMeterValue(&leftMeter,val,fgColor); 
    setMeterValue(&rightMeter,val,fgColor); 
    updateLedMeter(&leftMeter,millis());
    updateLedMeter(&rightMeter,millis()); 
    FastLED.show();     
}

int getMeterVal(){
  return config.hp;
}

void blinkLedsWithColor(int numBlinks, CRGB color, int blink_rate_millis){
  //TODO: refactor! led meter should be able to blink on its own without blocking
  //and we shouldnt have to know the max HP either
  int MAX_VAL = leftMeter.max_val;
  int val = MAX_VAL;
  for ( int i=0;i<numBlinks;i++){
    updateMeters(color, val);
    if ( val == 0 ){
       val = MAX_VAL;
    }        
    else{
      val = 0;
    }
    delayWithSound(blink_rate_millis);
  }
}


void POST(){
   Log.noticeln("POST...");
   blinkLedsWithColor(6, CRGB::Blue, 500 );
   Log.noticeln("POST COMPLETE");   
}

const char * trackerLifeStatus( LifeConfig &config ){
    if ( config.state == LifeStage::RESPAWNING){
        return "RESP";
    }
    else if ( config.state == LifeStage::DEAD){
        return "DEAD";
    }
    else if ( config.state == LifeStage::ALIVE){
        return "ALIV";
    }
    else{
        return "UNKN";
    }
}



//TODO: display update is so slow, it messes up the musing unless
//we give music a chance during the refresh
void updateDisplay(){  

  long m = millis();
  int secs = 0;
  if ( config.state == LifeStage::ALIVE){
    secs = secondsBetween(m, config.spawn_time); 
  }
  else if ( config.state == LifeStage::RESPAWNING){
    secs =secondsBetween(config.respawn_allowed_time_ms, m);
  }
  else if ( config.state == LifeStage::DEAD){
    secs = secondsBetween(m,config.death_time);
  }

  char SPACE = ' ';
  oled.setCursor(0,0);
  rtttl::play();
  oled.print(playerClassName(config.class_id)); oled.print(" HP: ");  oled.print(config.hp);  oled.print("/"); oled.print(config.max_hp); 
  oled.clearToEOL(); oled.println("");
  rtttl::play();
  oled.print("DMG: ");  oled.print(config.little_hit); oled.print("/"); oled.print(config.big_hit); 
  oled.clearToEOL(); oled.println("");
  rtttl::play();
  oled.print("STS: "); oled.print(trackerLifeStatus(config));oled.print(" (");oled.print(secs);oled.print(")");
  oled.clearToEOL(); oled.println("");
  oled.print("RESP: ");oled.print( config.respawnCount); oled.print(" Max: ");oled.print(config.maxLifeSpanSecs);
  oled.clearToEOL(); oled.println("");
}  

void printLifeConfig(){
  Log.noticeln("HP: %d/%d", config.hp,config.max_hp);
  Log.noticeln("Class: %d (%s)", config.class_id, playerClassName(config.class_id));
  rtttl::play();
  Log.noticeln("DMG: %d/%d", config.little_hit,config.big_hit);
  Log.noticeln("Respawn ms: %l",config.respawn_ms);
  rtttl::play();
  Log.noticeln("Respawn requested: %t",config.respawnRequested);

  long m = millis();
  Log.noticeln("Respawn allowed in %l ms", (config.respawn_allowed_time_ms-m));
  rtttl::play();
  Log.noticeln("STS: %s", trackerLifeStatus(config));
}


long getRespawnFlashInterval(long ms_left){
    // this will flash closer as we near respawn completion
    //30000 ms left = 1.5 sec flash
    //10000 ms left 0.5 sec flash
    //5000 ms left 0.25 sec flash
    int MULTIPLIER =20;
    return ms_left / MULTIPLIER;
}

void updateLEDs(){
  CRGB fg = CRGB::Green;
  long flashing = FlashInterval::FLASH_NONE;
  int meter_val = 0;
  if ( config.state == LifeStage::DEAD ){
    fg = CRGB::Red;
    flashing = FlashInterval::FLASH_SLOW;
    meter_val = config.max_hp;
    notifiedAlive = false;
  }
  else if ( config.state == LifeStage::RESPAWNING){
    fg = CRGB::Yellow;
    long time_left_to_respawn = (config.respawn_allowed_time_ms - millis());
    if ( time_left_to_respawn > 0 ){
      flashing = getRespawnFlashInterval(time_left_to_respawn);
    } 
    meter_val = config.max_hp;
  }
  else{
    if (! notifiedAlive  ){
        rtttl::begin(BUZZER_PIN, SOUND_LIFE1);
        notifiedAlive = true;
    }
    fg = CRGB::Green;
    flashing = FlashInterval::FLASH_NONE;
    meter_val = getMeterVal();
  }

  configureMeter(&leftMeter,config.max_hp, meter_val, fg, CRGB::Black);
  configureMeter(&rightMeter,config.max_hp, meter_val, fg, CRGB::Black);  
  leftMeter.flash_interval_millis = flashing;
  rightMeter.flash_interval_millis = flashing;
  long m = millis();
  updateLedMeter(&leftMeter, m );
  updateLedMeter(&rightMeter, m);

}

void setup(void) 
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.warning("Starting...");  
  pinMode(BUZZER_PIN, OUTPUT);
  //rtttl::begin(BUZZER_PIN, SOUND_LIFE1);
  Wire.begin();
  Wire.setClock(400000L);
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
  printLifeConfig();
}


void handleCard(LifeConfig &config){

  Log.noticeln("Before ReConfig: ");   printLifeConfig();
  int card_type = doc["card_type"];
  if ( ! card_type ){
    Log.warningln("Cannot detected Card Type.");
    return;
  }

  if ( card_type == NFCCardType::CLASS){
    if ( config.state == LifeStage::ALIVE){
        Log.infoln("Can't respawn while alive");
        rtttl::begin(BUZZER_PIN,CARD_NO_SCAN);  
        return;
    }
    else{
      Log.warningln("Handling Class Card");
    }

    int class_id = class_id["class_id"];
    
    if ( class_id){
      Log.infoln("Card class_id=%d ",class_id);
      config.class_id = doc["class_id"];
    }

    int max_hp = doc["max_hp"];
    if ( max_hp){
      Log.infoln("Card max_hp=%d ",max_hp);
      config.max_hp = doc["max_hp"];
    }
  
    int big_hit = doc["big_hit"];
    if ( big_hit){
      Log.infoln("Card big_hit=%d ",big_hit);
      config.big_hit = big_hit;
    }

    int little_hit = doc["little_hit"];
    if ( little_hit){
      Log.infoln("Card little_hit=%d ",little_hit);
      config.little_hit = little_hit;
    }

    long respawn_ms = doc["respawn_ms"];
    if ( respawn_ms){
      Log.infoln("Card respawn_ms=%l ",respawn_ms);
      config.respawn_ms = respawn_ms;
    }    

    long invuln_ms = doc["invuln_ms"];
    if ( invuln_ms){
      Log.infoln("Card invuln_ms=%l ",invuln_ms);
      config.invuln_ms = invuln_ms;
    }  

    requestRespawn(config, millis());      
  }
  else if ( card_type == NFCCardType::FLAG ){
    if ( config.state != LifeStage::ALIVE){
        Log.infoln("Flags only work when alive");
        rtttl::begin(BUZZER_PIN,CARD_NO_SCAN);  
        return;
    }
    else{
      Log.warningln("Handling Flag Card");
    }    
    Log.warning("picked up flag");
  }

  else if ( card_type == NFCCardType::MEDIC){
    if ( config.state != LifeStage::ALIVE){
        Log.infoln("Medic Cards only work when alive");
        rtttl::begin(BUZZER_PIN,CARD_NO_SCAN);  
        return;
    }
    else{
      Log.warningln("Handling Medic Card");
    }

    int hp_to_add = doc["add_hp"];
    if ( hp_to_add ){

       medic(config, hp_to_add);
    }
    else{
      Log.warning("No HP found");
    }
    
  }
  Log.noticeln("After Card: ");   printLifeConfig();
  notifiedDead = false;
  rtttl::begin(BUZZER_PIN, CARD_OK_SCAN);
  blinkLedsWithColor(6, CRGB::Blue, 200 );
  updateLife(config,millis());
  
  Log.warning("Reconfiguring DONE...");
}

 
void readNFC() 
{
 
 if (nfc.tagPresent(TAG_PRESENT_TIMEOUT))
 {
    NfcTag tag = nfc.read();
    tag.print();
    tagId = tag.getUidString();
    if ( tagId == lastTagId ){
      Log.warningln("Not reading tag %s again!",tagId);
      return;
    }
    else{
      lastTagId = tagId;
    }
    
    delayWithSound(AFTER_CARD_READ_DELAY_MS);
    
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
              handleCard(config);
            }
        } 
    
    }
 }
 else{
    lastTagId = "None";
 }
}


void checkDead(){
  if (! notifiedDead ){
      if ( config.state == LifeStage::DEAD){
          rtttl::begin(BUZZER_PIN,DEAD_MUSIC);
          notifiedDead = true;
      }
  }
  
}

void loop() 
{
    bigHitButton.loop();
    littleHitButton.loop(); 
    long m = millis();
    readNFC(); //handleCard could be called here
    updateLife(config,m);
    updateLEDs();
    if ( (m - last_display_update) > DISPLAY_UPDATE_INTERVAL_MS){
      updateDisplay();
      last_display_update = m;
    }
    
 
    checkDead();
    rtttl::play();
    FastLED.show();

}