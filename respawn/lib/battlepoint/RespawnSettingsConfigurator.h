#ifndef __INC_RESPAWNSETTINGSCONFIGURATOR_H
#define __INC_RESPAWNSETTINGSCONFIGURATOR_H
#include <RespawnTimer.h>
#include <RespawnSettings.h>

#define SECONDS_TO_MILLIS 1000
class RespawnSettingsConfigurator{

    public:
        RespawnSettingsConfigurator(RespawnSettings* settings){
            _settings = settings;
        };

        void setDefaults(){
            _settings->durations[0] = DEFAULT_SHORT_RESPAWN_MILLIS;
            _settings->durations[1] = DEFAULT_MEDIUM_RESPAWN_MILLIS;
            _settings->durations[2] = DEFAULT_LONG_RESPAWN_MILLIS;
        };

        void beginConfiguration( RespawnDurationSlot slotNum ){
            _slotNum = slotNum;
            _currentValue = 0;
        };
        int currentCount(){
            return _currentValue;
        };  
        void increment(int val){
            _currentValue += val;
        };

        void finishConfiguration(){
            _settings->durations[_slotNum] = _currentValue*SECONDS_TO_MILLIS;
        }

    protected:
        RespawnSettings* _settings;
        RespawnDurationSlot _slotNum ;
        int _currentValue = 0;

};
#endif