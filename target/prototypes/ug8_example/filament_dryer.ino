#define WHEEL_PIN 2
#define FAN_PIN 3
#define RELAY_PIN 0
#define DHTPIN 8
#define DHTTYPE DHT22
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


#include <U8g2lib.h>
#include <DHT.h>;
#include <Wire.h>

DHT dht(DHTPIN, DHTTYPE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R2); 

int HUMIDITY_SETPOINT=20;
int WHEEL_INTERVAL_SECONDS=240;

int hum;  
int tempF; 
char mode;
long lastTurnMillis = 0;

void setup() {
  Serial.begin(57600);
  Serial.println("Initializing..");
  Serial.print("Pin Map...");
    pinMode(WHEEL_PIN,OUTPUT);
    pinMode(FAN_PIN,OUTPUT);
    pinMode(RELAY_PIN,OUTPUT);
    digitalWrite(WHEEL_PIN,0);
    digitalWrite(FAN_PIN,0);
    digitalWrite(RELAY_PIN,0);
  Serial.print("[OK]");

  
  Serial.print("Temp Sensor...");
    dht.begin();
  Serial.print("[OK]");

  Serial.print("Display...");
    display.begin();
    display.setFont(u8g2_font_helvB14_tf);
  Serial.print("[OK]");    
  mode = 'W';
}

void incrementWheel(){
  const int MAX_POWER=200;
  const int STEPS=20;
  const int DELAY_US=500;
  for (int i=0;i<STEPS;i++){
    analogWrite(WHEEL_PIN,i*DELAY_US/STEPS);
    delayMicroseconds(DELAY_US);
  }
    for (int i=STEPS;i>0;i--){
    analogWrite(WHEEL_PIN,i*DELAY_US/STEPS);
    delayMicroseconds(DELAY_US);
  }
  analogWrite(WHEEL_PIN,0);
}

void readTemp(){
  hum = dht.readHumidity();
  tempF= dht.readTemperature(true);    
}

void updateDisplay(){  

  display.clearBuffer();
  display.setFont(u8g2_font_t0_11b_tf);
  display.setCursor(35,11);
  
  if ( mode == 'D' ){    
    display.print("DRYING");
  }
  else{
    display.print("IDLE");
  }
  
  display.setFont(u8g2_font_fub30_tn);
  display.setCursor(5,55);  
  display.print(tempF);
  display.setCursor(70,55);
  display.print(hum);
  display.setFont(u8g2_font_helvB14_tf  );
  display.setCursor(50,30);
  display.print(F("F"));
  display.setCursor(115,30);
  display.print(F("%"));
  display.sendBuffer();
}

void doDrying(){
  if ( hum > HUMIDITY_SETPOINT ){
      digitalWrite(FAN_PIN,1);
      digitalWrite(RELAY_PIN,1);
      mode = 'D';
      long timeSinceLastTurn = millis() - lastTurnMillis;
      if ( timeSinceLastTurn > WHEEL_INTERVAL_SECONDS*1000 ){
         incrementWheel();
         lastTurnMillis = millis();
      }      
  }
  else{
      digitalWrite(FAN_PIN,0);
      digitalWrite(RELAY_PIN,0);
      mode = 'W';
  }
}

void loop() {
  readTemp();
  doDrying();
  updateDisplay();
}