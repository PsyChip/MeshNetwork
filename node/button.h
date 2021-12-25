#ifndef Button_h
#define Button_h

#include "Arduino.h"

class Button {
  public:
    Button(int _pin, int treshold, int NCstate);
    boolean Poll();    // used for buttons
    boolean ReadPin(); // used for mechanical sensors
    
    int pin;
    boolean state;
    boolean nc;
    unsigned long idle;
    unsigned long hit;

  private:
    boolean _state;
    boolean lastState = LOW;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 300;

    unsigned long cps;
    int cfs;

};

#endif
