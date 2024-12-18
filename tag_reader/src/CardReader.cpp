#include <CardReader.h>
#include <PN532_HSU.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include "NFCCards.h"

#define AFTER_CARD_READ_DELAY_MS 1000
#define CARD_BUFFER_BYTES 500
#define TAG_PRESENT_TIMEOUT 100

#define CARDREAD_CONFIRM_NUM_BLINKS 6
#define CARDREAD_CONFIRM_BLINK_MS 500

HardwareSerial pn532_serial(2);
PN532_HSU pn532_s (pn532_serial);
NfcAdapter nfc = NfcAdapter(pn532_s);
byte nuidPICC[4];
String tagId= "None";
JsonDocument doc;

void cardReaderInit(){
  nfc.begin();
  Log.noticeln("NFC  [OK]");    
}


void parseClassCard ( ClassCard &card ){
  Log.infoln("Reading Class Card");
  
  cardSetDefaults(card );

  int max_hp = doc["max_hp"];
  if ( max_hp){
    card.max_hp = doc["max_hp"];
  }

  int big_hit = doc["big_hit"];
  if ( big_hit){
    card.big_hit = big_hit;
  }

  int little_hit = doc["little_hit"];
  if ( little_hit){
    card.little_hit = little_hit;
  }

  long invuln_ms = doc["invuln_ms"];
  if ( invuln_ms){
    card.invuln_ms = invuln_ms;
  }

  long respawn_ms = doc["respawn_ms"];
  if ( respawn_ms){
    card.respawn_ms = respawn_ms;
  }   

  unsigned char team = doc["team"];
  if ( team){
    card.team = team;
  }

  int player_class = doc["class"];
  if ( player_class){
    card.player_class = player_class;
  }
  Log.infoln("Read Class Card: class=%d max_hp=%d, bighit=%d, littlehit=%d, invuln_ms=%d, respawn_ms=%d, team=%c",
        card.player_class,card.max_hp,card.big_hit,card.little_hit,card.invuln_ms,card.respawn_ms,card.team);
}

void parseMedicCard ( MedicCard &card){
  Log.info("Reading Class Card");
  cardSetDefaults(card );
  int hp = doc["hp"];
  if ( hp){
    card.hp = hp;
  }
  Log.infoln("Read Medic Card: hp=%d",card.hp);
}

void parseFlagCard ( FlagCard &card){
  cardSetDefaults(card );
  Log.info("Reading Flag Card");  
  int value = doc["value"];
  if ( value){
    card.value = value;
  }

  unsigned char team = doc["team"];
  if ( team){
    card.team = team;
  }

  Log.infoln("Read Flag Card: team=%c, value=%d",card.team, card.team, card.value);
}

NFCCard parseCard(){

  NFCCard card;

  int card_type = doc["card_type"];
  if ( ! card_type ){
    Log.warning("Cannot detect Card Type.");
    card.type = NFCCardType::UNKNOWN;
    return card;
  }
  else{
    card.type = card_type;
  }

  if ( card_type == NFCCardType::CLASS ){
    parseClassCard ( card.classCard );
  }
  else if ( card_type == NFCCardType::FLAG){
    parseFlagCard ( card.flagCard );
  }
  else if ( card_type == NFCCardType::MEDIC){
    parseMedicCard ( card.medicCard );
  }
  return card;
}


NFCCard cardReaderTryCardIfPresent() 
{

  NFCCard card;
  cardSetDefaults(card);

  if (nfc.tagPresent(TAG_PRESENT_TIMEOUT))
  {
      NfcTag tag = nfc.read();
      tag.print();
      tagId = tag.getUidString();

      delay(AFTER_CARD_READ_DELAY_MS);
      
      if ( tag.hasNdefMessage() ){
          NdefMessage m = tag.getNdefMessage();
          if ( m.getRecordCount() > 0 ){
              NdefRecord r = m.getRecord(0);
              byte payload_buffer[CARD_BUFFER_BYTES];
              r.getPayload(payload_buffer);
              char* as_chars = (char*)payload_buffer;

              as_chars+=(3*sizeof(char)); //record has 'en in front of it, need to get rid of that.
              Log.traceln("Got Payload");Log.traceln(as_chars);
              DeserializationError error = deserializeJson(doc, as_chars);

              if (error) {
                  Log.errorln("deserializeJson() failed: ");
                  Log.errorln(error.f_str());
              }   
              else{
                  card = parseCard();
              } 
          } 
      }
  }
  else{
      card.type = NFCCardType::NONE;
  }
  
  return card;
}

