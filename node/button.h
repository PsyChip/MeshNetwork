#ifndef Button_h
#define Button_h

#include "Arduino.h"

class Button {
  public:
    Button(int _pin, int treshold,int NCstate);
    boolean Poll(unsigned long now);
    int pin;
    int state;
    int nc;
    unsigned long idle;
    unsigned long hit;
    
  private:
    int _state;
    int lastState = LOW;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 300;
};

#endif
