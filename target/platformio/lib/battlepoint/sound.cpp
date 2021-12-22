#include <sound.h>
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Teams.h>

HardwareSerial mySoftwareSerial(1);
DFRobotDFPlayerMini myDFPlayer;
SoundState soundState;
bool player_enabled =false;

void setup_sound_triggers(){
  for ( int i=0;i<NUM_SOUNDS;i++){}    
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_10SEC].game_secs_trigger = -10;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_1SEC].game_secs_trigger = -1;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_20SEC].game_secs_trigger = -20;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_2SEC].game_secs_trigger = -2;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_30SEC].game_secs_trigger = -30;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_3SEC].game_secs_trigger = -3;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_4SEC].game_secs_trigger = -4;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_5SEC].game_secs_trigger = -5;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_BEGINS_60SEC].game_secs_trigger = -60;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_10SEC].game_secs_trigger = 10;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_1SEC].game_secs_trigger = 1;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_20SEC].game_secs_trigger = 20;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_2MIN].game_secs_trigger = 120;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_2SEC].game_secs_trigger = 2;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_30SEC].game_secs_trigger = 30;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_3SEC].game_secs_trigger = 3;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_4SEC].game_secs_trigger = 4;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_5SEC].game_secs_trigger = 5;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_60SEC].game_secs_trigger = 60;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_6SEC].game_secs_trigger = 6;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_7SEC].game_secs_trigger = 7;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_8SEC].game_secs_trigger = 8;
    soundState.sound_config[SND_SOUNDS_ANNOUNCER_ENDS_9SEC].game_secs_trigger = 9;
   
}

int sound_times_played(int sound_id){
   return soundState.sound_config[sound_id].times_played;
}

void _init_dfplayer(int rx_pin, int tx_pin){
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

void sound_init_for_testing(){
  player_enabled = false;
  setup_sound_triggers();
}

void sound_init(int rx_pin, int tx_pin){
  _init_dfplayer(rx_pin,tx_pin);
  player_enabled = true;  
  setup_sound_triggers();
}

void reset_sounds_for_new_game(){
    //delay after the sound, in MS, and initally not played
    for ( int i=0;i<NUM_SOUNDS;i++){
      soundState.sound_config[i].last_played_ms = SOUND_CONFIG_NOT_PLAYED_YET;
      soundState.sound_config[i].times_played = 0;
    }
    
    //longer messages need more delay time afterwards
    soundState.sound_config[SND_SOUNDS_0023_ANNOUNCER_VICTORY].post_sound_delay_ms=2500;
}

long  millis_since_last_sound(long current_time_millis){
   return current_time_millis - soundState.last_sound_played_time_ms;
}

bool not_played_in_this_game(int sound_id){
   return soundState.sound_config[sound_id].last_played_ms == SOUND_CONFIG_NOT_PLAYED_YET;
}

bool ready_to_play(long current_time_millis){
   int prev_sound_delay= soundState.sound_config[soundState.last_sound_played_id].post_sound_delay_ms;
   return  millis_since_last_sound(current_time_millis) > prev_sound_delay;
}

void _log_sound_played(int sound_id, long current_time_millis){
  Serial.print(sound_id);Serial.println(":played");
  soundState.sound_config[sound_id].last_played_ms = current_time_millis;
  soundState.last_sound_played_time_ms = current_time_millis;
  soundState.sound_config[sound_id].times_played += 1;
}

void _play(int sound_id, long current_time_millis){
  if ( player_enabled ){
    myDFPlayer.play(sound_id);
  }  
  _log_sound_played(sound_id,current_time_millis);
}

void sound_play(int sound_id, long current_time_millis){
  if ( ready_to_play( current_time_millis) ){
    _play(sound_id,current_time_millis);
  }    
}

void sound_play_once_in_game(int sound_id, long current_time_millis){
  if ( ready_to_play( current_time_millis) && not_played_in_this_game(sound_id)){
    _play(sound_id,current_time_millis);
  }
  else{
    //Serial.print("Can't play right now:");Serial.println(sound_id);
  }
}

void sound_gametime_update ( int game_seconds_remaining , long current_time_millis){  
   for(int i=0;i<NUM_SOUNDS;i++){
       if ( game_seconds_remaining == soundState.sound_config[i].game_secs_trigger ){
          sound_play_once_in_game(i,current_time_millis);
       }
   }   
}

void sound_play_victory(Team winner,long current_time_millis){
    if ( winner == Team::BLU || winner == Team::RED){
        sound_play(SND_SOUNDS_0023_ANNOUNCER_VICTORY,current_time_millis);
    }  
    else{
        sound_play(SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10,current_time_millis);
    }
}

void play_random_startup(long current_time_millis){
    long r = random(18,25);
    sound_play(r,current_time_millis);
}
