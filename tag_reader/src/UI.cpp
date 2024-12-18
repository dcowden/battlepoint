#include <UI.h>
#include <FastLED.h>
#include <Wire.h>
#include <CardReader.h>
#include "gsl/gsl-lite.hpp"
#include <NonBlockingRtttl.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <Pins.h>
#define I2C_ADDRESS 0x3C   // 0X3C+SA0 - 0x3C or 0x3D
#define SIDE_LED_SIZE 8
#define MENU_FONT X11fixed7x14
#define LED_BRIGHTNESS 40
#define CARDREAD_CONFIRM_NUM_BLINKS 6
#define CARDREAD_CONFIRM_BLINK_MS 500
#define BATTLEPOINT_VERSION "1.1.3"
#define OLED_UPDATE_INTERVAL_MS 500

bool notifiedDead = false;
long last_oled_update = 0;

//extern LifeConfig currentConfig;

CRGB leftLeds[SIDE_LED_SIZE];
CRGB rightLeds[SIDE_LED_SIZE];
LedMeter leftMeter;
LedMeter rightMeter;
SSD1306AsciiWire oled;

const char * mario = "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
const char * SOUND_LIFE1 =  "life:d=8,o=4,b=450:e.5,32p.,g.5,32p.,e.6,32p.,c.6,32p.,d.6,32p.,g.6,32p.";
const char * LITTLE_HIT = "bighit:d=8,o=4,b=450:e.5,e.5,e.5";
const char * BIG_HIT = "little:d=8,o=4,b=450:8g,8g,8g";
const char * DEAD_MUSIC = "dead:d=32,o=4,b=64:32g,32p,32g,32p,32g,32p,2d#";

void blinkLedsWithColor(LifeConfig &config,int numBlinks, CRGB color, int blink_rate_millis);

void uiInit(LifeConfig &config, long current_time_millis){

    pinMode(Pins::BUZZER_PIN, OUTPUT);
        
    rtttl::begin(Pins::BUZZER_PIN, SOUND_LIFE1);
    FastLED.addLeds<NEOPIXEL, Pins::LEFT_LED_PIN >(leftLeds, SIDE_LED_SIZE);  
    FastLED.addLeds<NEOPIXEL, Pins::RIGHT_LED_PIN >(rightLeds, SIDE_LED_SIZE);   

    FastLED.setBrightness(LED_BRIGHTNESS);
    Log.noticeln("LEDS [OK]");  
    int IDX=SIDE_LED_SIZE-1;
    initMeter(&leftMeter,"left",leftLeds,IDX,0);
    initMeter(&rightMeter,"right",rightLeds,IDX,0);   
    Log.infoln("Meter Values: max=%d, val=%d",config.max_hp, config.hp);
    configureMeter(&leftMeter,config.max_hp,config.hp,CRGB::Green, CRGB::Black);

    oled.begin(&Adafruit128x64, I2C_ADDRESS);
    oled.setFont(MENU_FONT);
    oled.displayRemap(false);
    oled.clear();
    oled.println(BATTLEPOINT_VERSION); 
    oled.println("HitTracker...");
    Log.noticeln("OLED [OK]");         
    blinkLedsWithColor(config,6, CRGB::Blue, 500 ); 
    
    rtttl::play();   
}

void uiHandleLittleHit(long current_time_millis){
    rtttl::begin(Pins::BUZZER_PIN, BIG_HIT);
}

void uiHandleBigHit(long current_time_millis){
    rtttl::begin(Pins::BUZZER_PIN, LITTLE_HIT);
}

void uiDelay(LifeConfig &config,int millis_to_wait ){
  Expects(millis_to_wait > 0 );
  //delay while updating sound, leds, and display
  long s = millis();
  long end = millis_to_wait + s;
  while ( millis() < end){
      uiUpdate(config,s);
  }
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

void blinkLedsWithColor(LifeConfig &config, int numBlinks, CRGB color, int blink_rate_millis){
  long delay_ms = numBlinks* blink_rate_millis;
  updateMeters(color,leftMeter.max_val,blink_rate_millis);
  uiDelay(config,delay_ms);
} 

void updateDisplay(LifeConfig &config,long current_time_millis){  
  long start_time = millis();
  char SPACE = ' ';
  oled.setCursor(0,0);
  oled.print("HP: ");  oled.print(config.hp);  oled.print("/"); oled.print(config.max_hp); 
  oled.clearToEOL(); oled.println("");
  oled.print("CLS: "); oled.print(playerClassName(config.player_class)); oled.print( " TM: ");oled.print((char)(config.team));
  oled.clearToEOL(); oled.println("");
  oled.print("DMG: ");  oled.print(config.little_hit); oled.print("/"); oled.print(config.big_hit); 
  oled.clearToEOL(); oled.println("");
  oled.print("STS: "); oled.print(trackerLifeStatus(config));
  oled.clearToEOL(); oled.println("");
  //Log.traceln("Update Display :%l ms", millis()-start_time);
}  

long getRespawnFlashInterval(LifeConfig &config){
    // this will flash closer as we near respawn completion
    //30000 ms left = 1.5 sec flash
    //10000 ms left 0.5 sec flash
    //5000 ms left 0.25 sec flash
    int MULTIPLIER =20;
    return config.respawn_ms / MULTIPLIER;
}

void updateLEDs(LifeConfig &config,long current_time_millis){ 
    CRGB fgColor = CRGB::Black;
    long flash_interval = FlashInterval::FLASH_NONE;
    int meterVal = 0;
    const int METER_MAX = config.max_hp;
    if ( config.state == LifeStage::ALIVE){
      fgColor = CRGB::Green;
      flash_interval = FlashInterval::FLASH_NONE;
      meterVal = config.hp;
    }
    else if ( config.state == LifeStage::DEAD){
      fgColor = CRGB::Red;
      flash_interval = FlashInterval::FLASH_SLOW;
      meterVal = config.max_hp;
    }
    else if ( config.state == LifeStage::INVULNERABLE){
      fgColor = CRGB::Yellow;
      flash_interval = FlashInterval::FLASH_SLOW;
      meterVal = config.max_hp;
    }
    else if ( config.state == LifeStage::RESPAWNING){
      fgColor = CRGB::Orange;
      flash_interval = getRespawnFlashInterval(config);
      meterVal = config.max_hp;
    }
    else if ( config.state == LifeStage::INIT){
      //waiting to be configured
      fgColor = CRGB::Red;
      flash_interval = getRespawnFlashInterval(config);
      meterVal = config.max_hp;
    }
    else {
      //unknown state
      fgColor = CRGB::Black;
      flash_interval = FlashInterval::FLASH_NONE;
      meterVal=config.max_hp;      
    }

    setMeterValues(&leftMeter,meterVal, METER_MAX, fgColor,  flash_interval);
    setMeterValues(&rightMeter,meterVal, METER_MAX, fgColor,  flash_interval);    

    long m = millis();
    updateLedMeter(&leftMeter, m );
    updateLedMeter(&rightMeter, m);
    FastLED.show();
    //Log.traceln("Update Meters :%l ms", millis()-m);

}

void checkDead(LifeConfig &config){
  if (! notifiedDead ){
      if ( config.state == LifeStage::DEAD){
          rtttl::begin(BUZZER_PIN,DEAD_MUSIC);
          notifiedDead = true;
      }
  }
  
}

void uiHandleCardScanned( LifeConfig &config, long current_time_millis){
    blinkLedsWithColor(config,CARDREAD_CONFIRM_NUM_BLINKS, CRGB::Blue, CARDREAD_CONFIRM_BLINK_MS);
}

void uiUpdate(LifeConfig &config,long current_time_millis){
    rtttl::play();
    checkDead(config);
    if ( current_time_millis - last_oled_update > OLED_UPDATE_INTERVAL_MS ){
        updateDisplay(config, current_time_millis);
        last_oled_update = current_time_millis;
    }

    updateLEDs(config, current_time_millis);
}