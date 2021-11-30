#include <Arduino.h>
#include <avr/wdt.h>
#include "grid.h"
#include "heater.h"

#define __nodeId 1  // unique id, 0 for master

GridNode *n;
Heater *h;
unsigned long statetimer = 0;

ISR(WDT_vect) {
  h->Reset();
}

void Watchdog() {
  MCUSR = MCUSR & B11110111;
  WDTCSR = WDTCSR | B00011000;
  WDTCSR = B00100001;
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}

void StateChange(int state) {
  n->SendValue(0, sState, state);
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

void setup() {
  delay(50);
  Serial.begin(115200);
  Serial.println("init");
  h = new Heater();
  //n = new GridNode(__nodeId);
  //h->onStateChange = &StateChange;
  //n->onCommand = &RemoteCommand;
  Watchdog();
  Serial.println("begin");
}

void loop() {
  unsigned long now = millis();
  h->Cycle(now);

//  if ((now - statetimer) >= 250) {
    //n->Cycle(now);
    //statetimer = now;
    //    n->SendValue(0, sState, h->state);
  //}
  wdt_reset();
}
