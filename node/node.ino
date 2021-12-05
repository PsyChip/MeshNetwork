#include <Arduino.h>
#include <avr/wdt.h>
#include "grid.h"
#include "heater.h"

uint16_t manager = 00;
uint16_t NodeAddress = 01;

GridNode *n;
Heater *h;

unsigned long timer = 0;

void setup() {
  delay(100);
  h = new Heater();
  n = new GridNode(NodeAddress);
  h->onStateChange = &StateChange;

  n->onCommand = &RemoteCommand;
  n->Telemetry_(manager, sAc, 1);
  Watchdog();
}

void loop() {
  h->Cycle();
  n->Cycle();
  if ((millis() - timer) >= 15000) {
    timer = millis();
    n->Telemetry_(manager, sState, h->state);
  }
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
  n->Telemetry_(manager, sState, state);
}

void RemoteCommand(Command C) {
  switch (C.cmd) {
    case CmdSet: {
        switch (C.param) {
          case 1: {
              h->Start();
            }
            break;
          case 2: {
              h->Stop();
            }
            break;
          case 3: {
              h->Fill();
            }
        }
        break;
      }
      break;
    case CmdRain: {
        h->RainEx(C.param);
      }
      break;
    case CmdKill:
      {
        h->Reset();
      }
      break;
    case CmdStatus:
      {
        n->Telemetry_(manager, sState, h->state);
      }
      break;
  }
}

ISR(WDT_vect) {
  h->Reset();
}
