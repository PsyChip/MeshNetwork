#include <Arduino.h>
#include <avr/wdt.h>
#include "grid.h"
#include "heater.h"

uint16_t manager = 00;
uint16_t NodeAddress = 01;

GridNode *n;
Heater *h;

void setup() {
  delay(100);
  Serial.begin(115200);
  h = new Heater();
  n = new GridNode(NodeAddress);
  h->onStateChange = &StateChange;

  n->onCommand = &RemoteCommand;
  n->Telemetry_(manager, sAc, 1);
  Watchdog();
  Serial.println("begin");
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
  Serial.println("State changed");
  n->Telemetry_(manager, sState, state);
}

void RemoteCommand(Command C) {
  Serial.println(C.cmd);
  switch (C.cmd) {
    case CmdStart: {
        h->Start();
      }
      break;
    case CmdStop: {
        h->Stop();
      }
      break;
    case CmdRain: {
        h->RainEx(C.param);
      }
      break;
    case CmdReset:
      {
        h->Reset();
      }
      break;
  }
}

ISR(WDT_vect) {
  h->Reset();
}
