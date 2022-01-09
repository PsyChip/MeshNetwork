#include <Arduino.h>
#include "button.h"

Button::Button(int _pin, int treshold, int NCstate) {
  pin = _pin;
  pinMode(pin, INPUT_PULLUP);
  debounceDelay = treshold;
  nc = NCstate;
}

boolean Button::Poll() {
  unsigned long now = millis();
  state = digitalRead(pin);
  boolean res = false;
  if (state != lastState) {
    lastState = state;
    lastDebounceTime = now;
  }
  idle = (now - lastDebounceTime);
  if (idle >= debounceDelay) {
    if (state == nc && (now - hit) >= 500) {
      res = true;
      hit = now;
    }
  }
  return res;
}


boolean Button::ReadPin() {
  unsigned long now = millis();
  state = (digitalRead(pin) == nc ? true : false);
  if (((now - cps) >= debounceDelay) && state == cfs) {
    return state;
  }
  if (state != cfs) {
    cps = now;
    cfs = state;
  }
  return -1;
}
