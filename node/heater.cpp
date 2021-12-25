#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Arduino.h>
#include "heater.h"

void __HeaterOnStateChange(int state) {}

Heater::Heater() {
  pinMode(out_display, OUTPUT);
  pinMode(out_fill, OUTPUT);
  pinMode(out_drain, OUTPUT);
  pinMode(in_temp, INPUT);
  Reset();
  fill = new Button(in_fill, 100, LOW);
  drain = new Button(in_drain, 100, LOW);
  sensor = new Button(in_sensor, 2000, HIGH);

  onStateChange = &__HeaterOnStateChange;
  Reset();
}

void Heater::Start() {
  RainEx(false);
  digitalWrite(out_fill, LOW);
  state = 1;
}

void Heater::Fill() {
  if (state != sIdle) {
    return ;
  }
  digitalWrite(out_fill, LOW);
  state = sFillOnly;
}

void Heater::Reset() {
  digitalWrite(out_fill, HIGH);
  digitalWrite(out_display, HIGH);
  digitalWrite(out_drain, HIGH);
  state = sIdle;
}

void Heater::Stop() {
  digitalWrite(out_fill, HIGH);
  digitalWrite(out_display, HIGH);
  state = sIdle;
}

void Heater::Toggle() {
  if (state == sIdle) {
    Start();
  } else {
    Stop();
  }
}

void Heater::Rain() {
  Stop();
  if (digitalRead(out_drain) == HIGH) {
    digitalWrite(out_drain, LOW);
    state = sRain;
  } else {
    digitalWrite(out_drain, HIGH);
    state = sIdle;
  }
}

void Heater::RainEx(bool _status) {
  Stop();
  if (_status == true) {
    digitalWrite(out_drain, LOW);
    state = sRain;
  } else {
    digitalWrite(out_drain, HIGH);
    state = sIdle;
  }
}

unsigned long Heater::idle() {
  return millis() - StateChange;
}

void Heater::ReadTemp() {
  if (digitalRead(out_display) == HIGH) {
    temperature = TempOff;
    return ;
  }
  temperature = (digitalRead(in_temp) == HIGH ? TempCold : TempHot);
}

void Heater::ReadSensors() {
  ReadTemp();
  isFull = sensor->ReadPin();
}

void Heater::CheckFallBack() {
  if (idle() >= state_interval) {
    if ((state == 2 || state == 3) && isFull == false) {
      digitalWrite(out_display, HIGH);
      digitalWrite(out_fill, LOW);
      state = sFill;
      return ;
    }
  }
}

void Heater::Cycle() {
  if (fill->Poll() == true) {
    Toggle();
  }
  if (drain->Poll() == true) {
    Rain();
  }

  ReadSensors();
  CheckFallBack();

  switch (state) {
    case sIdle:
      {}
      // standby
      break;
    case sFill: {
        // filling
        if (isFull == true) {
          digitalWrite(out_fill, HIGH);
          digitalWrite(out_display, LOW);
          state = sHeat;
        }
      }
      break;
    case sHeat: {
        if (temperature == TempHot) {
          state = 3;
        }
      }
      break;
    case sReady: {
        if (temperature == TempCold) {
          state = sHeat;
        }
      }
      // ready
      break;
    case sRain:
      {}
      // draining
      break;
    case sFillOnly:
      // Fill & return back to standby
      {
        if (isFull == true) {
          digitalWrite(out_fill, HIGH);
          state = sIdle;
        }
      }
      break;
  }

  if (state != _state) {
    StateChange = millis();
    _state = state;
    onStateChange(state);
  }

}
