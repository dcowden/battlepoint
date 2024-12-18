#pragma once

#include <FastLED.h>
#include <LedMeter.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "HitTracker.h"

void uiInit(LifeConfig &config, long current_time_millis);
void uiHandleLittleHit(long current_time_millis);
void uiHandleBigHit(long current_time_millis);
void uiHandleCardScanned( LifeConfig &config, long current_time_millis);
void uiUpdate(LifeConfig &config,long current_time_millis);

//TODO: really dont like that even delay means 
//needing a config. 
// we can simplify this module by storing current state
//but that complicates testing
//both are better than accessing lifeconfig via extern
void uiDelay(LifeConfig &config,int millis_to_wait );