#pragma once

#include <FastLED.h>
#include <LedMeter.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

void initDisplay();
void cardScanned();
void handleLittleHit();
void handleBigHit();

void updateUI(long current_time_millis);
void uiDelay(int millis_to_wait );