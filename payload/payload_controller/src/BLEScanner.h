#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>
#define DEFAULT_RSSI_THRESHOLD -60
#define SCAN_TIME_SEC 1
#define DEBUG 1
class BLEProximityScanner{
    public:
        BLEProximityScanner();
        void init();
        void update();
        boolean foundDevice();
        int rssiThreshold;
    
    private:
        boolean deviceFound;
        BLEScan* bleScan;
};