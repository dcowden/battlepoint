#include "BLEScanner.h"

BLEProximityScanner::BLEProximityScanner(){
    deviceFound=false; 
    rssiThreshold = DEFAULT_RSSI_THRESHOLD;
}
void BLEProximityScanner::init(){
    //Serial.println("BLE: Begin Init.");
    BLEDevice::init("");
    //Serial.println("BLE: Init Done.");
    bleScan= BLEDevice::getScan();
    bleScan->setActiveScan(true); 
    bleScan->setInterval(100);
    bleScan->setWindow(99); 
}

void BLEProximityScanner::update(){
    
    boolean tmpFound = false;
    BLEScanResults foundDevices = bleScan->start(SCAN_TIME_SEC, false);
    for (int i=0;i< foundDevices.getCount();i++){
        BLEAdvertisedDevice advertisedDevice = foundDevices.getDevice(i);
        if ( DEBUG ){
            Serial.print("BLE:");
            Serial.print(advertisedDevice.getName().c_str());
            Serial.print(" ");
            Serial.println(advertisedDevice.getRSSI());
        }        
        if ( advertisedDevice.getRSSI() > rssiThreshold ){
            tmpFound  = true;

            //trigger finding a device as quickly as possible
            //this will trigger finding a device immediately, but waits a full scan cycle
            //to show it falling off the radar
            //Serial.print("BLE: found ");
            //Serial.println(advertisedDevice.getName().c_str());
            deviceFound = true;
            //Serial.printf("BLE::Close Device: %s \n", advertisedDevice.getName().c_str());
        }
    } 
    deviceFound = tmpFound;
    bleScan->clearResults(); 
}

boolean BLEProximityScanner::foundDevice(){
    return deviceFound;
} 


