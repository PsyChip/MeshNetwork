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
  pinMode(out_heat, OUTPUT);
  pinMode(out_drain, OUTPUT);
  Reset();
  fill = new Button(in_fill, 100, LOW);
  drain = new Button(in_drain, 100, LOW);
  sensor = new Button(in_sensor, 500, HIGH);

  onStateChange = &__HeaterOnStateChange;
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
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

unsigned long Heater::idle() {
  return millis() - StateChange;
}

void Heater::ReadTemp() {
  if (digitalRead(out_display) == HIGH) {
    temperature = TempOff;
    return ;
  } 
  int tmp = analogRead(in_temp);
  total = total - readings[readIndex];
  readings[readIndex] = tmp;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  average = total / numReadings;
 //Serial.println(average);
  if (average >= temp_upper && temperature != TempCold) {
    temperature = TempCold;
    Serial.println("cold");
    Serial.println(average);
  }

  if (average <= temp_lower && temperature != TempHot) {
    temperature = TempHot;
    Serial.println("hot");
    Serial.println(average);
  }
}

void Heater::ReadSensors() {
  ReadTemp();
  isFull = sensor->ReadPin(millis());
}

void Heater::CheckFallBack() {
  if (idle() >= state_interval) {
    if ((state == 2 || state == 3) && isFull == false) {
      digitalWrite(out_heat, HIGH);
      digitalWrite(out_fill, LOW);
      state = 1;
      return ;
    }

    if ((state == 3) && (temperature == TempCold)) {
      digitalWrite(out_heat, LOW);
      state = 2;
      return ;
    }
  }
}

void Heater::Cycle() {
  unsigned long now = millis();
  if (fill->Poll(now) == true) {
    Toggle();
  }
  if (drain->Poll(now) == true) {
    Rain();
  }

  ReadSensors();
  CheckFallBack();

  switch (state) {
    case 0:
      // standby
      break;
    case 1:
      // filling
      if (isFull == true) {
        digitalWrite(out_fill, HIGH);
        digitalWrite(out_heat, LOW);
        state = 2;
      }
      break;
    case 2:
      // heating
      if (temperature == TempHot) {
        digitalWrite(out_heat, HIGH);
        state = 3;
      }
      break;
    case 3:
      // ready
      break;
    case 4:
      // draining
      break;
  }

  if (state != _state) {
    StateChange = now;
    _state = state;
    onStateChange(state);
  }

}
