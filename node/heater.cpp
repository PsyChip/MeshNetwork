#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Arduino.h>
#include "heater.h"

void __HeaterOnStateChange(int state) {}

Heater::Heater() {
  pinMode(in_sensor, INPUT_PULLUP);
  pinMode(in_temp, INPUT);
  pinMode(out_display, OUTPUT);
  pinMode(out_fill, OUTPUT);
  pinMode(out_heat, OUTPUT);
  pinMode(out_drain, OUTPUT);
  Reset();
  fill = new Button(in_fill, 100, LOW);
  drain = new Button(in_drain, 100, LOW);
  onStateChange = &__HeaterOnStateChange;
  Reset();
}

void Heater::Start() {
  RainEx(false);
  digitalWrite(out_fill, LOW);
  digitalWrite(out_display, LOW);
  state = 1;
}

void Heater::Reset() {
  digitalWrite(out_heat, HIGH);
  digitalWrite(out_fill, HIGH);
  digitalWrite(out_display, HIGH);
  digitalWrite(out_drain, HIGH);
  state = 0;
}

void Heater::Stop() {
  digitalWrite(out_heat, HIGH);
  digitalWrite(out_fill, HIGH);
  digitalWrite(out_display, HIGH);
  state = 0;
}

void Heater::Toggle() {
  if (state == 0) {
    Start();
  } else {
    Stop();
  }
}

void Heater::Rain() {
  Stop();
  if (digitalRead(out_drain) == HIGH) {
    digitalWrite(out_drain, LOW);
    state = 4;
  } else {
    digitalWrite(out_drain, HIGH);
    state = 0;
  }
}

void Heater::RainEx(bool _status) {
  Stop();
  if (_status == true) {
    digitalWrite(out_drain, LOW);
    state = 4;
  } else {
    digitalWrite(out_drain, HIGH);
    state = 0;
  }
}

unsigned long Heater::StateTook() {
  return millis() - StateChange;
}

void Heater::Cycle(unsigned long now) {

  if (fill->Poll(now) == true) {
    Toggle();
  }
  if (drain->Poll(now) == true) {
    Rain();
  }

  full = ((digitalRead(in_sensor) == HIGH) ? true : false);
  hot = ((digitalRead(in_temp) == HIGH) ? true : false);

  switch (state) {
    case 0: // standby
      break;
    case 1: // filling
      if (full == true) {
        digitalWrite(out_fill, HIGH);
        digitalWrite(out_heat, LOW);
        state = 2;
      }
      break;
    case 2: // heating
      if (hot == true) {
        digitalWrite(out_heat, HIGH);
        state = 3;
      }
      break;
    case 3: // ready
      if (full == false) {
        digitalWrite(out_fill, LOW);
        state = 1;
        return ;
      }

      if (hot == false) {
        digitalWrite(out_heat, LOW);
        state = 2;
        return;
      }

      break;
    case 4: // draining
      break;
  }

  if (state != _state) {
    StateChange = now;
    onStateChange(state);
    _state = state;
  }

}
