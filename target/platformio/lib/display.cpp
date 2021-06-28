#include <Arduino.h>
#include <U8g2lib.h>
#include <display.h>



/**
void initDisplay(U8G2_SSD1306_128X64_NONAME_F_HW_I2C* oled){
  oled->begin();
  displayWelcomeBanner(oled);
  oled->clear();
  oled->setFont(u8g2_font_6x13_tf);
}

void displayWelcomeBanner( U8G2_SSD1306_128X64_NONAME_F_HW_I2C* oled){
  oled->clearBuffer();
  oled->firstPage();
  oled->setFontPosBaseline();
  oled->setFont(u8g2_font_logisoso16_tf);
  oled->setCursor(0,40);
  oled->print("BP Target v0.2");   
  oled->sendBuffer();
  delay(SPLASH_WAIT_MS);    
}**/