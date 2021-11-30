#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Arduino.h>
#include "button.h"

Button::Button(int _pin, int treshold) {
  pin = _pin;
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
  debounceDelay = treshold;
}

boolean Button::Poll(unsigned long now) {
  state = digitalRead(pin);
  boolean res = false;
  if (state != lastState) {
    lastDebounceTime = now;
  }
  idle = (now - lastDebounceTime);
  if (idle >= debounceDelay) {
    if (state != _state) {
      _state = state;
      if (_state == LOW) {
        res = true;
      }
    }
  }
  lastState = state;
  return res;
}
