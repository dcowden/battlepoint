class Proximity {
  public:
     Proximity(GameOptions* gameOptions, uint8_t redButtonPin, uint8_t bluButtonPin);
     boolean isRedClose();
     boolean isBluClose();
     void redButtonPress();
     void bluButtonPress();
     void debugStatus();
  private:
     boolean _is_red_close;
     boolean _is_blu_close;
     uint8_t _redButtonPin;
     uint8_t _bluButtonPin;
     long last_btn_red_change_millis;
     long last_btn_blu_change_millis;
     int _timeThresholdSeconds;
     GameOptions* _gameOptions;
};