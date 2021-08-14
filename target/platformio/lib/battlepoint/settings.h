#ifndef __INC_SETTINGS_H
#define __INC_SETTINGS_H
#include <EEPROM.h>
#define BP_VERSION 102
#define CONFIG_START 0

//esp32 flash is 512 bytes

/*
  OLD CODE
  void loadSettings(){
  
  SettingsData data = {
     //overall options
     BP_VERSION,
     DFPLAYER_VOLUME,
     BRIGHTNESS,
     
     //game options
     DEFAULT_GAME_TIME,
     DEFAULT_MODE,
     DEFAULT_CAPTURE_SECONDS,
     DEFAULT_BUTTON_SECONDS,
     DEFAULT_START_DELAY
  };

  if ( EEPROM.read(CONFIG_START) == BP_VERSION ){
    for (unsigned int t=0; t<sizeof(data); t++)
      *((char*)&data + t) = EEPROM.read(CONFIG_START + t);    
  }  


  BETTER WAY
        #include <EEPROM.h>

        struct MyObject{
        float field1;
        byte field2;
        char name[10];
        };

        void setup(){

        float f = 0.00f;   //Variable to store data read from EEPROM.
        int eeAddress = 0; //EEPROM address to start reading from

        Serial.begin( 9600 );
        while (!Serial) {
            ; // wait for serial port to connect. Needed for Leonardo only
        }
        Serial.print( "Read float from EEPROM: " );

        //Get the float data from the EEPROM at position 'eeAddress'
        EEPROM.get( eeAddress, f );
        Serial.println( f, 3 );  //This may print 'ovf, nan' if the data inside the EEPROM is not a valid float.

        // get() can be used with custom structures too.
        eeAddress = sizeof(float); //Move address to the next byte after float 'f'.
        MyObject customVar; //Variable to store custom object read from EEPROM.
        EEPROM.get( eeAddress, customVar );

        Serial.println( "Read custom object from EEPROM: " );
        Serial.println( customVar.field1 );
        Serial.println( customVar.field2 );
        Serial.println( customVar.name );
        }  
*/
#endif