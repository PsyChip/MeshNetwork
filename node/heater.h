#ifndef Heater_h
#define Heater_h

#include "Arduino.h"
#include "Button.h"

#define in_fill 5
#define in_drain 6
#define in_sensor 7
#define in_temp A5

#define out_display A0
#define out_fill A1
#define out_heat A3
#define out_drain A2

#define temp_upper 300
#define temp_lower 120
#define numReadings 32

#define state_interval 3000 // don't let application switch states so fast

PROGMEM enum {
  CmdStart = 1,
  CmdStop = 2,
  CmdRain = 3,
  CmdReset = 4,
  CmdStatus = 5,
  CmdIdle = 6,

  TempOff = 0,
  TempCold = 1,
  TempHot = 2
};

class Heater {
  public:
    Heater();
    void Cycle();
    void Start();
    void Stop();
    void Rain();
    void RainEx(bool _status);
    void Reset();
    int state = 0;
    unsigned long idle();      // calculates idle of state
    unsigned long StateChange = 0;  // used for calculate idle
    void (*onStateChange)(int state);
  private:
    void ReadSensors();
    void CheckFallBack();
    void Toggle();
    void ReadTemp();
    Button *fill;
    Button *drain;
    Button *sensor;
    int _state = 0;
    int temperature;
    bool isFull;

    // ###############################################
    int readings[numReadings];      // the readings from the analog input
    int readIndex = 0;              // the index of the current reading
    int total = 0;                  // the running total
    int average = 0;                // the average
    // ###############################################
};

#endif
