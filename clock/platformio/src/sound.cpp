#include <sound.h>
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"

HardwareSerial mySoftwareSerial(1);
DFRobotDFPlayerMini myDFPlayer;

void sound_init(int rx_pin, int tx_pin){
  mySoftwareSerial.begin(9600, SERIAL_8N1, rx_pin, tx_pin);  // speed, type, RX, TX
  Serial.begin(115200);
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    
    Serial.println(myDFPlayer.readType(),HEX);
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms  
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);  
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
}
void sound_play(int sound_id){
    myDFPlayer.play(sound_id);
}
void play_random_startup(){
    long r = random(18,25);
    sound_play(r);
}