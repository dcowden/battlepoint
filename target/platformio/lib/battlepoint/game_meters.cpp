#include <game_meters.h>
#include <Teams.h>

void updateLeds(MeterSettings* meters, long current_time_millis ){
    updateLedMeter(meters->left,current_time_millis);
    updateLedMeter(meters->right,current_time_millis);
    updateLedMeter(meters->center,current_time_millis);
    updateLedMeter(meters->leftTop,current_time_millis);
    updateLedMeter(meters->leftBottom,current_time_millis);
    updateLedMeter(meters->rightTop,current_time_millis);
    updateLedMeter(meters->rightBottom,current_time_millis);
}
void setFlashMeterForTeam(Team t, MeterSettings* meters, FlashInterval fi ){
    if ( t == Team::RED ){
        meters->leftBottom->flash_interval_millis = fi;
        meters->leftTop->flash_interval_millis = fi;        
    }
    else  if ( t == Team::BLU ){
        meters->rightBottom->flash_interval_millis = fi;
        meters->rightTop->flash_interval_millis = fi;        
    }
    else{
        Log.warningln("Ignored updating flash on meters for invalid team.");
    }
}

void setMetersToFlashInterval(MeterSettings* meters, long flashInterval){
    meters->center->flash_interval_millis = flashInterval;
    meters->leftBottom->flash_interval_millis = flashInterval;
    meters->leftTop->flash_interval_millis = flashInterval;
    meters->left->flash_interval_millis = flashInterval;
    meters->rightBottom->flash_interval_millis = flashInterval;
    meters->rightTop->flash_interval_millis = flashInterval;
    meters->right->flash_interval_millis = flashInterval;
}

void setHorizontalMetersToTeamColors(MeterSettings* meters){
    setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
    setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Red, CRGB::Black );
    setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );
    setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Blue, CRGB::Black );  
}

void setHorizontalMetersToNeutralColors(MeterSettings* meters){
    setMeterValues( meters->leftTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Yellow, CRGB::Black );
    setMeterValues( meters->leftBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Yellow, CRGB::Black );
    setMeterValues( meters->rightTop, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Yellow, CRGB::Black );
    setMeterValues( meters->rightBottom, STANDARD_METER_MAX_VAL, STANDARD_METER_MAX_VAL, CRGB::Yellow, CRGB::Black );  
}