#include <Arduino.h>
#include <U8g2lib.h>
#include <game.h>
#include <pins.h>
#include <game.h>

#define U8_WIDTH 128
#define U8_HEIGHT 64
#define SPLASH_WAIT_MS 1000
#define WINNER_SPLASH_MS 1000

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0,  I2C_SDA,I2C_SCL);

typedef struct {
    double vBatt=0.0;
    const char* version;
} HardwareInfo;

typedef struct{
    int fontH;
    int fontW;
} DisplaySettings;

short fontW = 6;
short fontH = 13;

void displayWelcomeBanner(const char* version ){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.setCursor(0,30);
  oled.print("BattlePoint");
  oled.setCursor(0,50); 
  oled.print("v");    
  oled.print(version);
  oled.sendBuffer();
  delay(SPLASH_WAIT_MS);    
}

void initDisplay(){
  oled.setBusClock(2000000);
  oled.begin();
  oled.clear();
  oled.setFlipMode(0);
  oled.setFont(u8g2_font_6x13_tf);
}

void gameOverDisplay(GameState gameState){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.setCursor(0,40);
  oled.print("WIN: "); 
  oled.setCursor(50,40);  
  oled.print(teamTextChar(gameState.result.winner));
  oled.sendBuffer();
  delay(WINNER_SPLASH_MS);
  oled.setFont(u8g2_font_7x13_mf);   
}

void diagnosticsDisplay(HardwareInfo hwi){
  oled.clearBuffer();
  oled.firstPage();
  oled.setCursor(5,15);
  oled.print("VERSION: ");oled.print(hwi.version);
  oled.setCursor(5,27);
  oled.print("  VBATT: ");oled.print(hwi.vBatt,2);
  oled.setFont(u8g2_font_7x13_mf); 
  oled.sendBuffer();  
}

void updateDisplay(GameState gameState, GameSettings gameSettings){
  oled.clearBuffer();
  if ( gameSettings.gameType == GameType::GAME_TYPE_TARGET_TEST){
    oled.setCursor(5,15);
    oled.print("HT: R="); oled.print(gameState.redHits.hits); oled.print( "  B="); oled.print(gameState.bluHits.hits);
    oled.setCursor(5,27);
    oled.print("TRIG: "); oled.print(gameSettings.target.trigger_threshold); 
    oled.setCursor(5,39);
    oled.print("LAST: "); oled.print(gameState.lastHit.last_hit_energy);oled.print(" ");oled.print(gameState.lastHit.singleSampleTimeMillis,2);oled.print(" ");oled.print(gameState.lastHit.peak0);
    oled.setCursor(5,52);
    oled.print(gameState.lastHit.last_hit_energy);oled.print(" ");oled.print(gameState.lastHit.overall_avg_energy);oled.print(" ");oled.print(gameState.lastHit.avg_energy_bins[5]);oled.print(" ");oled.print(gameState.lastHit.avg_energy_bins[8]);
  }
  else{
    oled.setCursor(5,15);
    oled.print("HT: R="); oled.print(gameState.redHits.hits); oled.print( "  B="); oled.print(gameState.bluHits.hits);

    long elapsed_millis = millis() - gameState.time.start_time_millis;
    int elapsed_sec = elapsed_millis/1000;
    oled.setCursor(5,27);
    oled.print("T: "); oled.print(elapsed_sec); oled.print("/"); oled.print(gameSettings.timed.max_duration_seconds); 
    oled.print(" ["); oled.print(getCharForStatus(gameState.status)); oled.print("]");
    oled.setCursor(5,39);
    oled.print("CAP: "); oled.print(gameState.ownership.capture_hits); oled.print("/"); oled.print(gameSettings.capture.hits_to_capture);
    oled.setCursor(5,52);
    oled.print("OWN: B=");oled.print(gameState.ownership.blu_millis/1000);oled.print("  R="); oled.print(gameState.ownership.red_millis/1000);
  }
  oled.sendBuffer();
  
}
