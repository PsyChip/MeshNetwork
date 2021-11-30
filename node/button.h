#ifndef Button_h
#define Button_h

#include "Arduino.h"

class Button {
  public:
    Button(int _pin, int treshold);
    boolean Poll(unsigned long now);
    int pin;
    int state;
    unsigned long idle;
  private:
    int _state;
    int lastState = LOW;
    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 300;
};

#endif
