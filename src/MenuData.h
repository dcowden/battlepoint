#ifndef _bpMenu_
#define _bpMenu_
#include "MenuManager.h"
#include <avr/pgmspace.h>

/*
Generated using LCD Menu Builder at http://lcd-menu-bulder.cohesivecomputing.co.uk/.
*/

enum bpMenuCommandId
{
  mnuCmdBack = 0,
  mnuCmdmode,
  mnuCmdcap,
  mnuCmdtime,
  mnuCmdstart,
  mnuCmdtapdur,
  mnuCmdVolume,
  mnuCmdBrightness,
  mnuCmdSaveSettings,
  mnuCmdstartdelay
};

PROGMEM const char bpMenu_back[] = "Back";
//PROGMEM const char bpMenu_exit[] = "Exit";

PROGMEM const char bpMenu_1[] = "1. Game Mode";
PROGMEM const char bpMenu_2[] = "2. Cap Seconds";
PROGMEM const char bpMenu_3[] = "3. Game Time";
PROGMEM const char bpMenu_4[] = "4. Tap Duration";
PROGMEM const char bpMenu_5[] = "5. Start Delay";
PROGMEM const char bpMenu_6[] = "6. Volume";
PROGMEM const char bpMenu_7[] = "7. Brightness";
PROGMEM const char bpMenu_8[] = "8. Save Settings";
PROGMEM const char bpMenu_9[] = "9. Start Game";

PROGMEM const MenuItem bpMenu_Root[] = {{mnuCmdmode, bpMenu_1}, {mnuCmdcap, bpMenu_2}, 
      {mnuCmdtime, bpMenu_3}, {mnuCmdtapdur, bpMenu_4}, {mnuCmdstartdelay,bpMenu_5},
       {mnuCmdVolume, bpMenu_6},{mnuCmdBrightness, bpMenu_7},{mnuCmdSaveSettings, bpMenu_8},{mnuCmdstart, bpMenu_9}};


#endif
