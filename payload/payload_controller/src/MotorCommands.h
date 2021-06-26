#ifndef MotorCommands_h
#define MotorCommands_h

const byte MOTOR_CONTROLLER_ADDRESS = 42;

struct MotorCommand{
  float leftVelocity;
  float rightVelocity;
  byte enabled;
};

typedef union  MotorPacket_t
{
    float velocity;
    byte velocityBytes[4];
};

#endif
