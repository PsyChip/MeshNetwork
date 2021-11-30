#include <Arduino.h>
#include <avr/wdt.h>
#include "grid.h"
#include "parser.h"

#define __nodeId 0  // unique id, 0 for master

GridNode *n;
Parser *p;

void Watchdog() {
  MCUSR = MCUSR & B11110111;
  WDTCSR = WDTCSR | B00011000;
  WDTCSR = B00100001;
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}

void ProcessInput(int CmdId) {
  switch (CmdId) {
    case 1:
      p->splitParamInt();
      n->SendCommand(p->paramsInt[0], p->paramsInt[1], p->paramsInt[2]);
      break;
    case 2:
      n->SendPing(p->paramInt());
      break;
    case 3:
      n->List();
      break;
  }
}

void setup() {
  delay(50);
  Serial.begin(115200);
  n = new GridNode(__nodeId);
  p = new Parser();
  p->onCommand = &ProcessInput;
  Watchdog();
}

void loop() {
  unsigned long now = millis();
  n->Cycle(now);
  p->Poll(now);
  wdt_reset();
}
