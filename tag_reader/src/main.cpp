#include <Arduino.h>
#include <ArduinoLog.h>
#include "HitTracker.h"
#include <Controls.h>
#include <UI.h>
#include <CardReader.h>
#include "gsl/gsl-lite.hpp"
#include <Pins.h>


long last_ui_update = 0;
const long ui_update_interval=500L;

LifeConfig currentConfig;
const long WIRE_CLOCK_SPD = 100000;
void onBigHit(){
  if ( currentConfig.state == LifeStage::ALIVE ){  
    long m = millis();
    Log.traceln("LittleHit");
    trackerBigHit(currentConfig, m);
    trackerStartInvuln(currentConfig, m);
    uiHandleBigHit(m);
  }
}

void onLittleHit(){
  if ( currentConfig.state == LifeStage::ALIVE ){
    long m = millis();    
    Log.traceln("BigHit");
    trackerLittleHit(currentConfig,m );
    trackerStartInvuln(currentConfig, m);
  }  
}

void setup(void) 
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_INFO, &Serial, true);
  Log.warning("Starting...");  
  Wire.begin();
  Wire.setClock(400000L);
  Log.noticeln("WIRE [OK]");
  initInputs();
  registerLittleHitHandler(onLittleHit);
  registerBigHitHandler(onBigHit);
  Log.noticeln("BTNS [OK]");
  trackerInit(currentConfig, millis()); 
  uiInit(currentConfig, millis() );
  cardReaderInit();
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
    trackerRequestRespawn(currentConfig, millis() );    
}

void handleMedicCard( MedicCard &card ){
   trackerApplyMedic(currentConfig, card.hp, millis() );
}

void handleFlagCard ( FlagCard &card){
  Log.warning("picked up flag");
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

void loop() 
{
  NFCCard card = cardReaderTryCardIfPresent();
  if ( card.type != NFCCardType::NONE){
    handleCard(card);
    uiHandleCardScanned(currentConfig, millis() );
 }

  updateInputs(); 
  trackerUpdateLifeModel(currentConfig, millis() ); //updating the model
  uiUpdate(currentConfig, millis() );

}