#ifndef POSE_H
#define POSE_H
#include "MotorCommands.h"
#include "pinmap.h"

//app contants
#define NUM_ADC_SAMPLES 20
#define MAX_SPEED 6000.0
#define MAX_MENU_SPEED 4000
#define MIN_FORWARD_SPEED 1000.0
#define MIN_SPEED -6000.0
#define SENSOR_CLOSE 2500
#define SENSOR_CENTERED 100
#define SLOW_SPEED 2000.0
#define ENDGAME_OVERTIME_MAX_NON_POSSESSION_MILLIS 5000 //how long can elapse with attackers not possessing

#define P_GAIN 1.0
#define D_GAIN 0.02

#define DEBUG 0

struct gameOptions
{
    //stuff in here gets stored to the EEPROM
    int firmwareVersion;  // Firmware version to check if we need to overwrite
    int16_t gameTimeSeconds;        
    int16_t fwdSpeed_1;      
    int16_t fwdSpeed_2;    
    int16_t fwdSpeed_3; 
    int16_t bwdSpeed_1; 
    int16_t rssiThreshold;         
};


class Payload{
  public:
    Payload();
    void update();  
    struct gameOptions options;  
    int fwd_btn_1;
    int fwd_btn_2;
    int fwd_btn_3;
    int bwd_btn_1;
    int left_sensor;
    int right_sensor;
    float lastError;
    int getNumberForwardButtonsPressed();
    boolean isMoving();
    boolean isContested();
    boolean isDefended();
    void enable();
    void disable();
    MotorCommand lastCommand; 
    boolean manualDrive;   
    boolean enabled;

  private:
    int readButton(int pin);
    int computeNominalSpeed();
    int readADCPinPeak(int pin,int num_samples);
    int num_fwd_pressed();
    void setVelocity(float left, float right);
    void setBothVelocity(float both);
    void computeManualDrive();
    void computeInGameDrive();
    void stopAllMotors();

};

class Game{
  public:
    Game(Payload* _payload);
    int getSecondsRemaining();
    int durationSeconds;
    boolean isOver();
    boolean isOverTime();
    void start();
    struct gameOptions options;
  private:
    long startTime;    
    Payload* payload;
    long lastNonPossessionTime;
    boolean overTime;
    boolean running;
    boolean shouldEndGame();
};

#endif
