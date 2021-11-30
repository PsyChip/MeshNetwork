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
#define out_heat A2
#define out_drain A3

enum {
  CmdStart = 1,
  CmdStop = 2,
  CmdRain = 3,
  CmdReset = 4,
  CmdStatus = 5,
  CmdIdle = 6
};

class Heater {
  public:
    Heater();
    void Cycle(unsigned long now);
    void Start();
    void Stop();
    void Rain();
    void RainEx(bool _status);
    void Reset();
    int state = 0;
    unsigned long StateTook();      // calculates idle of state
    unsigned long StateChange = 0;  // used for calculate idle
    void (*onStateChange)(int state);
  private:
    void Toggle();
    Button *fill;
    Button *drain;
    Button *sensor;
    int _state = 0;
    bool hot = false;
    bool full = false;
};

#endif
