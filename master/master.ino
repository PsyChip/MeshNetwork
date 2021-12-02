#include <Arduino.h>
#include <avr/wdt.h>
#include "grid.h"
#include "parser.h"

GridNode *n;
Parser *p;

#define _sec 1000
#define _comma F(",")
#define _end F(";")

void Watchdog() {
  MCUSR = MCUSR & B11110111;
  WDTCSR = WDTCSR | B00011000;
  WDTCSR = B00100001;
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}

PROGMEM const uint16_t __nodeID = 00;

void setup() {
  delay(100);
  Serial.begin(115200);
  Serial.println("init");
  n = new GridNode(__nodeID);
    Serial.println("node init");
  p = new Parser();
  n->onAck = &ProcAck;
  n->onPong = &ProcPong;
  n->onTelemetry = &ProcTelemetry;
  
  p->onCommand = &ProcessInput;
  Watchdog();
  Serial.println("ready");
}

void loop() {
  n->Cycle();
  p->Poll();
  wdt_reset();
}

void ProcessInput(int CmdId) {
  switch (CmdId) {
    case 1:
      p->splitParamInt();
      bool R=n->Send(p->paramsInt[0], p->paramsInt[1], p->paramsInt[2]);
      Serial.println(R);
       bool P=n->Ping_(p->paramInt());
      Serial.println(P);
      break;
    case 2:
    Serial.print("Sending to:");
    Serial.println(p->paramInt());
//      bool P=n->Ping_(01);
//      Serial.println(P);
      break;
    case 3:
      break;
  }
}

void ProcAck(Ack A) {
  Serial.print(F("$ack,"));
  Serial.print(A.sender);
  Serial.print(_comma);
  Serial.print(A.cmdId);
  Serial.print(_comma);
  Serial.print(A.result);
  Serial.println(_end);
}

void ProcPong(Pong P) {
  Serial.print(F("$node,"));
  Serial.print(P.sender);
  Serial.print(_comma);
  Serial.print(P.ttl);
  Serial.print(_comma);
  Serial.print(P.uptime);
  Serial.println(_end);
}

void ProcTelemetry(Telemetry T) {
  Serial.print(F("$app,"));
  Serial.print(T.sender);
  Serial.print(_comma);
  Serial.print(T.type);
  Serial.print(_comma);
  Serial.print(T.value);
  Serial.println(_end);
}

void ProcCommand(Command C) {
Serial.print(F("$cmd,"));
  Serial.print(C.sender);
  Serial.print(_comma);
  Serial.print(C.cmd);
  Serial.print(_comma);
  Serial.print(C.param);
  Serial.println(_end);

}
