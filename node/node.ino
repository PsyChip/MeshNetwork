#include <Arduino.h>
#include <avr/wdt.h>
#include "grid.h"
#include "heater.h"

uint16_t address = 01;

GridNode *n;
Heater *h;

void setup() {
  h = new Heater();
  n = new GridNode(address);
  h->onStateChange = &StateChange;
  n->onCommand = &RemoteCommand;
  Watchdog();
}

void loop() {
  h->Cycle();
  n->Cycle();
  wdt_reset();
}

void Watchdog() {
  MCUSR = MCUSR & B11110111;
  WDTCSR = WDTCSR | B00011000;
  WDTCSR = B00100001;
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}

void StateChange(int state) {
  n->Telemetry_(0, sState, state);
}

void RemoteCommand(Command C) {
  switch (C.cmd) {
    case CmdStart:
      h->Start();
      break;
    case CmdStop:
      h->Stop();
      break;
    case CmdRain:
      h->RainEx(C.param);
      break;
    case CmdReset:
      h->Reset();
      break;
  }
}

ISR(WDT_vect) {
  h->Reset();
}
