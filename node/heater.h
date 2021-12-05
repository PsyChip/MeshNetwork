#ifndef Heater_h
#define Heater_h

#include "Arduino.h"
#include "Button.h"

#define in_fill 5
#define in_drain 6
#define in_sensor 7
#define in_temp 8

#define out_display A0
#define out_fill A1
#define out_drain A2

#define temp_upper 300
#define temp_lower 120
#define state_interval 3000 // don't let application switch states so fast

enum {
  CmdSet = 1,
  CmdRain = 2,
  CmdKill = 3,
  CmdStatus = 4,
  
  TempOff = 0,
  TempCold = 1,
  TempHot = 2,

  sIdle = 0,
  sFill = 1,
  sHeat = 2,
  sReady = 3,
  sRain = 4,
  sFillOnly = 5
};

class Heater {
  public:
    Heater();
    void Cycle();
    void Start();
    void Stop();
    void Rain();
    void Fill();
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
};

#endif
