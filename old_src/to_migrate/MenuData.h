#ifndef _bpMenu_
#define _bpMenu_
#include "MenuManager.h"


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

const char bpMenu_back[] = "Back";
//PROGMEM const char bpMenu_exit[] = "Exit";

const char bpMenu_1[] = "1. Game Mode";
const char bpMenu_2[] = "2. Cap Seconds";
const char bpMenu_3[] = "3. Game Time";
const char bpMenu_4[] = "4. Tap Duration";
const char bpMenu_5[] = "5. Start Delay";
const char bpMenu_6[] = "6. Volume";
const char bpMenu_7[] = "7. Brightness";
const char bpMenu_8[] = "8. Save Settings";
const char bpMenu_9[] = "9. Start Game";
const MenuItem bpMenu_Root[] = {{mnuCmdmode, bpMenu_1}, {mnuCmdcap, bpMenu_2}, 
      {mnuCmdtime, bpMenu_3}, {mnuCmdtapdur, bpMenu_4}, {mnuCmdstartdelay,bpMenu_5},
       {mnuCmdVolume, bpMenu_6},{mnuCmdBrightness, bpMenu_7},{mnuCmdSaveSettings, bpMenu_8},{mnuCmdstart, bpMenu_9}};


#endif
