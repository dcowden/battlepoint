#include <Arduino.h>
#include <U8g2lib.h>
#include <pins.h>
#include <GameClock.h>

#define U8_WIDTH 128
#define U8_HEIGHT 64
#define SPLASH_WAIT_MS 1000
#define WINNER_SPLASH_MS 5000

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

void gameOverDisplay(){
  oled.clearBuffer();
  oled.firstPage();
  oled.setFontPosBaseline();
  oled.setFont(u8g2_font_logisoso16_tf);
  oled.setCursor(0,40);
  oled.print("OVER: "); 
  oled.sendBuffer();
  delay(WINNER_SPLASH_MS);
  oled.setFont(u8g2_font_7x13_mf);   
}

void updateDisplay(GameClockState* clockState,HardwareInfo* hwi){
  oled.clearBuffer();

  oled.setCursor(5,15);
  oled.print("Elapsed:  "); oled.print(clockState->game_elapsed_secs);
  oled.setCursor(5,27);
  oled.print("Remaining:  "); oled.print(clockState->game_remaining_secs);
  oled.setCursor(5,39);
  oled.println(get_state_desc(clockState->clockState));
  oled.setCursor(5,51);
  oled.print("VBATT:  "); oled.print(hwi->vBatt,2);
  oled.sendBuffer();
} 
