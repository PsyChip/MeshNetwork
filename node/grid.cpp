#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Arduino.h>
#include "grid.h"

RF24 radio(mesh_pin_1, mesh_pin_2);
RF24Network network(radio);

PROGMEM const unsigned long crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

PROGMEM const size_t szTelemetry = sizeof(Telemetry);
PROGMEM const size_t szCommand = sizeof(Command);
PROGMEM const size_t szPing = sizeof(Ping);
PROGMEM const size_t szAck = sizeof(Ack);

void __GridNodeonCommandDef(Command C) {}
void __GridNodeonPongDef(Pong p) {}
void __GridNodeonAckDef(Ack a) {}
void __GridNodeonTelemetryDef(Telemetry t) {}

GridNode::GridNode(uint16_t nodeId) {
  nodeID = nodeId;
  radio.setChannel(mesh_channel);
  radio.begin();
  network.begin(nodeID);
  
  onCommand = &__GridNodeonCommandDef;
  onPong = &__GridNodeonPongDef;
  onAck = &__GridNodeonAckDef;
  onTelemetry = &__GridNodeonTelemetryDef;
}

void GridNode::Cycle() {
  network.update();

  while (network.available()) {
    Receive();
  }
}

void GridNode::Ack_(uint16_t addr, int cid, int result) {
  Ack a = {nodeID, cid, result};
  RF24NetworkHeader _header(addr, idAck);
  network.write(_header, &a, szAck);
}

void GridNode::Receive() {
  RF24NetworkHeader header;
  network.peek(header);
  switch (header.type) {
    case idPing:
      Ping p;
      network.read(header, &p, szPing);
      if (p.pong == true && nodeID == 0) {
        int ttl = (micros() - p.nanosec);
        Pong G = {header.from_node, ttl, p.uptime};
        onPong(G);
      } else {
        p.pong = true;
        p.uptime = millis();
        RF24NetworkHeader _header(header.from_node, idPing);
        network.write(_header, &p, szPing);
      }
      break;
    case idCommand:
      Command c;
      network.read(header, &c, szCommand);
      ProcessCommand(c, header.from_node);
      break;
    case idAck:
      Ack z;
      network.read(header, &z, szAck);
      onAck(z);
      break;
  }
}

void GridNode::Send(uint16_t address, int type, int value) {
  Telemetry payload = {type, value};
  RF24NetworkHeader header(address, idTelemetry);
  network.write(header, &payload, szTelemetry);
}

void GridNode::Ping_(uint16_t address) {
  RF24NetworkHeader header(address, idPing);
  Ping payload = {micros(), 0, false};
  network.write(header, &payload, szPing);
}

int GridNode::Command_(uint16_t addr, int cmd, unsigned long param) {
  RF24NetworkHeader header(addr, idCommand);
  Command payload = {CommandNr, nodeID, cmd, param};
  unsigned long __crc = BuildCRC(payload);
  payload.crc = __crc;
  if (network.write(header, &payload, szCommand)) {
    CommandNr++;
    return 1;
  } else {
    return 0;
  }
}

unsigned long GridNode::BuildCRC(Command cmd) {
  _crc[0] = cmd.id;
  _crc[1] = cmd.sender;
  _crc[2] = cmd.cmd;
  _crc[3] = network_key;

  long crc = ~0L;
  for (int index = 0 ; index < 4  ; ++index) {
    crc = crc_table[(crc ^ _crc[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (_crc[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

void GridNode::ProcessCommand(Command cmd, uint16_t sender) {
  unsigned long crc = BuildCRC(cmd);
  if (crc != cmd.crc) {
    Ack_(sender, cmd.id, AckKey);
    return ;
  }
  if (cmd.crc == lastMsg) {
    Ack_(sender, cmd.id, AckRep);
    return ;
  }
  lastMsg = cmd.crc;
  onCommand(cmd);
  Ack_(sender, cmd.id, AckOK);
}
