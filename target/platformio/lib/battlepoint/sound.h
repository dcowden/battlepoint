#ifndef __INC_SOUND_H
#define __INC_SOUND_H
#include <Teams.h>

#define SND_SOUNDS_0001_ANNOUNCER_ALERT    1 /* sounds/0001_announcer_alert.mp3 */
#define SND_SOUNDS_0002_ANNOUNCER_ALERT_CENTER_CONTROL_BEING_CONTESTED    2 /* sounds/0002_announcer_alert_center_control_being_contested.mp3 */
#define SND_SOUNDS_0014_ANNOUNCER_LAST_FLAG   3 /* sounds/0014_announcer_last_flag.mp3 */
#define SND_SOUNDS_0015_ANNOUNCER_LAST_FLAG2    4 /* sounds/0015_announcer_last_flag2.mp3 */
#define SND_SOUNDS_0016_ANNOUNCER_OVERTIME    5 /* sounds/0016_announcer_overtime.mp3 */
#define SND_SOUNDS_0017_ANNOUNCER_OVERTIME2   6 /* sounds/0017_announcer_overtime2.mp3 */
#define SND_SOUNDS_0018_ANNOUNCER_SD_MONKEYNAUT_END_CRASH02   7 /* sounds/0018_announcer_sd_monkeynaut_end_crash02.mp3 */
#define SND_SOUNDS_0019_ANNOUNCER_STALEMATE   8 /* sounds/0019_announcer_stalemate.mp3 */
#define SND_SOUNDS_0020_ANNOUNCER_SUCCESS   9 /* sounds/0020_announcer_success.mp3 */
#define SND_SOUNDS_0021_ANNOUNCER_TIME_ADDED    10 /* sounds/0021_announcer_time_added.mp3 */
#define SND_SOUNDS_0022_ANNOUNCER_TOURNAMENT_STARTED4   11 /* sounds/0022_announcer_tournament_started4.mp3 */
#define SND_SOUNDS_0023_ANNOUNCER_VICTORY   12 /* sounds/0023_announcer_victory.mp3 */
#define SND_SOUNDS_0024_ANNOUNCER_WARNING   13 /* sounds/0024_announcer_warning.mp3 */
#define SND_SOUNDS_0025_ANNOUNCER_WE_CAPTURED_CONTROL   14 /* sounds/0025_announcer_we_captured_control.mp3 */
#define SND_SOUNDS_0026_ANNOUNCER_WE_LOST_CONTROL   15 /* sounds/0026_announcer_we_lost_control.mp3 */
#define SND_SOUNDS_0027_ANNOUNCER_YOU_FAILED    16 /* sounds/0027_announcer_you_failed.mp3 */
#define SND_SOUNDS_0028_ENGINEER_SPECIALCOMPLETED10   17 /* sounds/0028_engineer_specialcompleted10.mp3 */
#define SND_SOUNDS_0030_GAMESTARTUP2    18 /* sounds/0030_gamestartup2.mp3 */
#define SND_SOUNDS_0031_GAMESTARTUP4    19 /* sounds/0031_gamestartup4.mp3 */
#define SND_SOUNDS_0032_GAMESTARTUP5    20 /* sounds/0032_gamestartup5.mp3 */
#define SND_SOUNDS_0033_GAMESTARTUP6    21 /* sounds/0033_gamestartup6.mp3 */
#define SND_SOUNDS_0034_GAMESTARTUP7    22 /* sounds/0034_gamestartup7.mp3 */
#define SND_SOUNDS_0035_GAMESTARTUP8    23 /* sounds/0035_gamestartup8.mp3 */
#define SND_SOUNDS_0036_GAMESTARTUP15   24 /* sounds/0036_gamestartup15.mp3 */
#define SND_SOUNDS_0037_GAMESTARTUP16   25 /* sounds/0037_gamestartup16.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_10SEC   26 /* sounds/announcer_begins_10sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_1SEC    27 /* sounds/announcer_begins_1sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_20SEC   28 /* sounds/announcer_begins_20sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_2SEC    29 /* sounds/announcer_begins_2sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_30SEC   30 /* sounds/announcer_begins_30sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_3SEC    31 /* sounds/announcer_begins_3sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_4SEC    32 /* sounds/announcer_begins_4sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_5SEC    33 /* sounds/announcer_begins_5sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_BEGINS_60SEC   34 /* sounds/announcer_begins_60sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_10SEC   35 /* sounds/announcer_ends_10sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_1SEC    36 /* sounds/announcer_ends_1sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_20SEC   37 /* sounds/announcer_ends_20sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_2MIN    38 /* sounds/announcer_ends_2min.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_2SEC    39 /* sounds/announcer_ends_2sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_30SEC   40 /* sounds/announcer_ends_30sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_3SEC    41 /* sounds/announcer_ends_3sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_4SEC    42 /* sounds/announcer_ends_4sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_5SEC    43 /* sounds/announcer_ends_5sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_60SEC   44 /* sounds/announcer_ends_60sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_6SEC    45 /* sounds/announcer_ends_6sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_7SEC    46 /* sounds/announcer_ends_7sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_8SEC    47 /* sounds/announcer_ends_8sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_ENDS_9SEC    48 /* sounds/announcer_ends_9sec.mp3 */
#define SND_SOUNDS_ANNOUNCER_TIME_ADDED   49 /* sounds/announcer_time_added.mp3 */
#define NUM_SOUNDS 50
#define SOUND_TRIGGER_NEVER -999
#define DEFAULT_POST_SOUND_DELAY 900
#define SOUND_CONFIG_NOT_PLAYED_YET 0

typedef struct {
    int  post_sound_delay_ms = DEFAULT_POST_SOUND_DELAY;
    bool last_played_ms = SOUND_CONFIG_NOT_PLAYED_YET;
    int  game_secs_trigger = SOUND_TRIGGER_NEVER; //>0, trigger with seconds remaining. <0, trigger on time before game starts
    int  times_played = 0; //for testing. raelly we need a dfplayer mock but i dont know how to do that
} SoundConfig;

typedef struct {
    long last_sound_played_time_ms = 0;
    int last_sound_played_id = 0;
    SoundConfig  sound_config[NUM_SOUNDS];
} SoundState;

int sound_times_played(int sound_id);
void sound_init(int rx_pin, int tx_pin);
void sound_init_for_testing();
void reset_sounds_for_new_game();
void sound_play(int sound_id);
void sound_play(int sound_id,long current_time_millis);
void sound_play_once_in_game(int sound_id, long current_time_millis);
void sound_play_victory(Team winner,long current_time_millis);
void sound_play_random_startup(long current_time_millis);
void sound_gametime_update ( int seconds_remaining, long current_time_millis ); //intended to be called every 200 ms or so
#define STATE_PLAYING 512

#endif
