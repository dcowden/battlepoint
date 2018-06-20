Proximity::Proximity(GameOptions* gameOptions,uint8_t redButtonPin, uint8_t bluButtonPin){
   last_btn_red_change_millis = -1;
   last_btn_blu_change_millis = -1;
   _redButtonPin = redButtonPin;
   _bluButtonPin = bluButtonPin;
   _gameOptions = gameOptions;
};
boolean Proximity::isRedClose(){
  uint8_t  btn = digitalRead(_redButtonPin);
  if ( btn == 0 ){
     return true;
  }
  return last_btn_red_change_millis > 0 &&
          (millis() - last_btn_red_change_millis) < (_gameOptions->getCaptureButtonThresholdSeconds() *1000);
};
boolean Proximity::isBluClose(){
  uint8_t  btn = digitalRead(_bluButtonPin);
  if ( btn == 0 ){
     return true;
  }  
  return last_btn_blu_change_millis > 0 &&
         (millis() - last_btn_blu_change_millis) < (_gameOptions->getCaptureButtonThresholdSeconds() *1000);

};

void Proximity::debugStatus(){
  #ifdef BP_DEBUG
  Serial.print(F("CaptureThreshold="));
  Serial.println(_gameOptions->getCaptureButtonThresholdSeconds() *1000);
  Serial.print(F("Red Pressed"));
  Serial.println((last_btn_red_change_millis));
  Serial.print(F("Blu Pressed"));
  Serial.println((last_btn_blu_change_millis));
  Serial.print(F("Blue Is Close="));
  Serial.print(isBluClose());
  Serial.print(F(", Red Is Close="));
  Serial.println(isRedClose());
  #endif
};
void Proximity::redButtonPress(){
    last_btn_red_change_millis = millis();
};

void Proximity::bluButtonPress(){
    last_btn_blu_change_millis = millis();
};