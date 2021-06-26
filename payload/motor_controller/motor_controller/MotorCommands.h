#ifndef MotorCommands_h
#define MotorCommands_h

const byte MOTOR_CONTROLLER_ADDRESS = 42;

struct Motor_Command{
  float leftVelocity;
  float rightVelocity;
  byte enabled;
};

#endif
