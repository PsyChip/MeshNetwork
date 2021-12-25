#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Arduino.h>
#include "grid.h"

const size_t szTelemetry = sizeof(Telemetry);
const size_t szCommand = sizeof(Command);
const size_t szPing = sizeof(Ping);
const size_t szAck = sizeof(Ack);

const unsigned long crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

void __GridNodeonCommandDef(Command c) {}
void __GridNodeonPongDef(Pong p) {}
void __GridNodeonAckDef(Ack a) {}
void __GridNodeonTelemetryDef(Telemetry t) {}

RF24 radio(mesh_pin_1, mesh_pin_2);
RF24Network network(radio);

GridNode::GridNode(uint16_t address) {
  _address = address;
  radio.begin();

  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_MAX);
  radio.setRetries(1, 50);

  network.begin(mesh_channel, address);
  network.multicastRelay = true;
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

void GridNode::flush() {
  radio.flush_tx();
  delay(1);
  radio.flush_rx();
}

void GridNode::Ack_(uint16_t address, unsigned int cid,  unsigned int result) {
  Ack a = {_address, cid, result};
  RF24NetworkHeader _header(address, idAck);
  network.write(_header, &a, szAck);
}

void GridNode::Receive() {
  RF24NetworkHeader header;
  network.peek(header);
  switch (header.type) {
    case idPing: {
        Ping p;
        network.read(header, &p, szPing);
        if (p.pong == true && _address == 0) {
          unsigned int ttl = (micros() - p.nanosec);
          Pong G = {header.from_node, ttl, p.uptime};
          onPong(G);
        } else {
          p.pong = true;
          p.uptime = millis();
          RF24NetworkHeader _header(header.from_node, idPing);
          network.write(_header, &p, szPing);
        }
      }
      break;
    case idCommand: {
        Command c;
        network.read(header, &c, szCommand);
        ProcessCommand(c, header.from_node);
      }
      break;
    case idAck: {
        Ack z;
        network.read(header, &z, szAck);
        onAck(z);
      }
      break;
    case idTelemetry: {
        Telemetry T;
        network.read(header, &T, szTelemetry);
        onTelemetry(T);
      }
      break;
    default: {
        byte data[256];
        network.read(header, &data, sizeof(data));
        Serial.print("$unk,");
        Serial.println(header.type);
        Serial.print(",");
        for (int i = 0; i < 256; i++) {
          Serial.print(char(data[i]));
          Serial.print(" ");
        }
        Serial.println(";");
      }
      break;
  }
}

boolean GridNode::Telemetry_(uint16_t address, unsigned int type, unsigned long value) {
  Telemetry payload = {_address, type, value};
  RF24NetworkHeader header(address, idTelemetry);
  return network.write(header, &payload, szTelemetry);
}

boolean GridNode::Ping_(uint16_t address) {
  RF24NetworkHeader header(address, idPing);
  Ping payload = {micros(), 0, false};
  return network.write(header, &payload, szPing);
}

boolean GridNode::Command_(uint16_t address, unsigned int cmd, unsigned long param) {
  RF24NetworkHeader header(address, idCommand);
  Command payload = {count_, _address, cmd, param, 0};
  payload.crc = BuildCRC(payload);
  if (network.write(header, &payload, szCommand)) {
    count_++;
    return true;
  } else {
    return false;
  }
}

unsigned long GridNode::BuildCRC(Command cmd) {
  _crc[0] = cmd.id;
  _crc[1] = cmd.sender;
  _crc[2] = cmd.cmd;
  _crc[3] = network_key;

  long crc = ~0L;
  for (int index = 0 ; index < 4 ; ++index) {
    crc = crc_table[(crc ^ _crc[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (_crc[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

void GridNode::ProcessCommand(Command cmd, uint16_t sender) {
  if (cmd.sender != sender) {
    Ack_(sender, cmd.id, AckSpf);
    return ;
  }
  if (cmd.crc == lastCmd) {
    Ack_(sender, cmd.id, AckRep);
    return ;
  }
  unsigned long crc = BuildCRC(cmd);
  if (crc != cmd.crc) {
    Ack_(sender, cmd.id, AckKey);
    return ;
  }
  Ack_(sender, cmd.id, AckOK);
  lastCmd = cmd.crc;
  cmd.sender = sender;
  onCommand(cmd);
}
