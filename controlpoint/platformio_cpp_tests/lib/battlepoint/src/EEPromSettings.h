#ifndef __INC_EEPROM_SETTINGS_H
#define __INC_EEPROM_SETTINGS_H
#include <Game.h>

SettingsData getDefaultSettings();
SettingsData loadSettings();
void saveSettings(SettingsData data );
#endif