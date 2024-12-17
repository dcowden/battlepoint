#include <Arduino.h>
#include <ArduinoLog.h>
#include <TeamNFC.h>
#include <Controls.h>
#include <UI.h>
#include <Card.h>
#include "gsl/gsl-lite.hpp"
#include <Pins.h>

LifeConfig currentConfig;

void onBigHit(){
  if ( currentConfig.state == LifeStage::ALIVE ){  
    Log.traceln("LittleHit");
    big_hit(currentConfig, millis());
    start_invuln(currentConfig, millis());
  }
}

void onLittleHit(){
  if ( currentConfig.state == LifeStage::ALIVE ){
    Log.traceln("BigHit");
    little_hit(currentConfig,millis() );
    start_invuln(currentConfig, millis());
  }  
}

void setup(void) 
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.warning("Starting...");  
  Wire.begin();
  Wire.setClock(100000);
  Log.noticeln("WIRE [OK]");

  initInputs();
  registerLittleHitHandler(onLittleHit);
  registerBigHitHandler(onBigHit);
  Log.noticeln("BTNS [OK]");

  initDisplay();
  initNFC();
  Log.noticeln("DONE [OK]");  
}


void handleClassCard( ClassCard &card ){
    Log.warningln("Class Card");
    
    currentConfig.max_hp = card.max_hp;
    currentConfig.big_hit = card.big_hit;
    currentConfig.little_hit = card.little_hit;
    currentConfig.respawn_ms = card.respawn_ms;
    currentConfig.invuln_ms = card.invuln_ms;
    currentConfig.player_class = card.player_class;
    currentConfig.team = card.team;
    requestRespawn(currentConfig, millis() );    
}

void handleMedicCard( MedicCard &card ){
   medic(currentConfig, card.hp, millis() );
}

void handleFlagCard ( FlagCard &card){
  Log.warning("picked up flag");
}

void handleRespawnCard(NFCCard &card){
  Log.warningln("Requesting Respawn for existing Class");
  requestRespawn(currentConfig, millis() );
}

void handleMedicCard(NFCCard &card){

}

void handleCard(NFCCard &card){
  if ( card.type == NFCCardType::CLASS){
    handleClassCard(card.classCard);
  }
  else if ( card.type == NFCCardType::MEDIC){
    handleMedicCard(card.medicCard);
  }
  else if ( card.type == NFCCardType::FLAG){
    handleFlagCard(card.flagCard);
  }
  else if ( card.type == NFCCardType::UNKNOWN){
    Log.warningln("Well I got a card, but i dont know what it is. Ignoring");
  }
}


//status:
//need to get rid of calls to TeamNFC functions from inside Card code:
// 1. split out LifeConfig and ClassConfig -- nfc is only reading a card
// 2. nfc should simply return struct of what we got-- either a card or whatever
//   2.1 need to define structs for that.  
// 3. main should update the model(s) and then update the UI

void loop() 
{
  NFCCard card = readNFC();
  if ( card.type != NFCCardType::NONE){
    handleCard(card);
  }


    /*
    if ( readNFC(currentConfig) ){
      cardScanned();
    }

    Something like
    ClassConfig newConfig;
    newConfig = readNFC();
    if ( newConfig.type = Config){
        requestRespawn()
    }
    else if (  newConfig.type == Medic)
        addPoints
    */

    updateInputs(); //react to button presses: updates mode and Ui 
    updateLifeStatus(currentConfig, millis() ); //updating the model
    updateUI(millis()); //updating the UI
}