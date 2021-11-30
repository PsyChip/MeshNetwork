#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <Arduino.h>
#include "grid.h"

RF24 radio(mesh_pin_1, mesh_pin_2);
RF24Network network(radio);
RF24Mesh mesh(radio, network);
rf24_datarate_e rate = RF24_1MBPS;

#define _sec 1000
#define _comma F(",")
#define _end F(";")

const unsigned long crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

void __GridNodeonCommandDef(Command C) {}

GridNode::GridNode(unsigned int nodeId) {
  nodeID = nodeId;
  if (nodeID > 0) {
    mesh.setChild(true);
  }
  radio.setPALevel(RF24_PA_HIGH);
  mesh.setNodeID(nodeID);
  mesh.begin(mesh_channel, rate, mesh_timeout);
  onCommand = &__GridNodeonCommandDef;
}

void GridNode::Cycle(unsigned long now) {
  mesh.update();
  if (nodeID == 0) {
    mesh.DHCP();
  }

  while (network.available()) {
    Receive();
  }

  if (nodeID == 0) {
    Broadcast(now);
  } else {
    CheckConnection(now);
  }
}

void GridNode::List() {
  Serial.print(F("$list,"));
  for (int i = 0; i < mesh.addrListTop; i++) {
    Serial.print(mesh.addrList[i].nodeID);
    Serial.print(F(":"));
    Serial.print(mesh.addrList[i].address, OCT);
    if (i < mesh.addrListTop - 1) {
      Serial.print(_comma);
    }
  }
  Serial.println(_end);
}

uint8_t GridNode::AddrToId(uint16_t addr) {
  for (int i = 0; i < mesh.addrListTop; i++) {
    if (mesh.addrList[i].address == addr) {
      return mesh.addrList[i].nodeID;
    }
  }
  return -1;
}

uint16_t GridNode::IdToAddr(uint8_t nodeId) {
  for (int i = 0; i < mesh.addrListTop; i++) {
    if (mesh.addrList[i].nodeID == nodeId) {
      return mesh.addrList[i].address;
    }
  }
  return -1;
}

void GridNode::SendAck(uint16_t addr, int cid, int result) {
  Ack a = {nodeID, cid, result};
  RF24NetworkHeader _header(addr, idAck);
  network.write(_header, &a, sizeof(a));
}

void GridNode::Receive() {
  RF24NetworkHeader header;
  network.peek(header);
  switch (header.type) {
    case idPing:
      Ping p;
      network.read(header, &p, sizeof(p));
      if (p.pong == true && nodeID == 0) {
        int ttl = (micros() - p.nanosec);

        Serial.print(F("$node,"));
        Serial.print(AddrToId(header.from_node));
        Serial.print(_comma);
        Serial.print(ttl);
        Serial.print(_comma);
        Serial.print(p.uptime);
        Serial.println(_end);

      } else {
        p.pong = true;
        p.uptime = millis();
        RF24NetworkHeader _header(header.from_node, idPing);
        network.write(_header, &p, sizeof(p));
      }
      break;
    case idCommand:
      Command c;
      network.read(header, &c, sizeof(c));
      ProcessCommand(c, header.from_node);
      break;
    case idAck:
      Ack z;
      network.read(header, &z, sizeof(z));
      Serial.print(F("$ack,"));
      Serial.print(z.sender);
      Serial.print(_comma);
      Serial.print(z.cmdId);
      Serial.print(_comma);
      Serial.print(z.result);
      Serial.println(_end);
      break;
  }
}

void GridNode::SendValue(uint16_t address, int type, int value) {
  Sensor payload = {type, value};
  RF24NetworkHeader header(address, idSensor);
  network.write(header, &payload, sizeof(payload));
}

void GridNode::SendPingEx(uint16_t address) {
  RF24NetworkHeader header(address, idPing);
  Ping payload = {micros(), 0, false};
  network.write(header, &payload, sizeof(payload));
}

void GridNode::SendPing(uint8_t _nodeId) {
  SendPingEx(IdToAddr(_nodeId));
}

int GridNode::SendCommand(int node, int cmd, unsigned long param) {
  uint16_t addr = IdToAddr(node);
  if (addr < 0) {
    return -1;
  }
  RF24NetworkHeader header(addr, idCommand);
  Command payload = {CommandNr, nodeID, cmd, param};
  unsigned long __crc = BuildCRC(payload);
  payload.crc = __crc;
  if (network.write(header, &payload, sizeof(payload))) {
    CommandNr++;
    return 1;
  } else {
    return 0;
  }
}

void GridNode::Broadcast(unsigned long now) {
  if ((now - timer[1]) >= mesh_conn_check) {
    timer[1] = now;
    for (int i = 0; i < mesh.addrListTop; i++) {
      SendPingEx(mesh.addrList[i].address);
    }
  }
}

void GridNode::CheckConnection(unsigned long now) {
  if ((now - timer[1]) >= mesh_conn_check) {
    timer[1] = now;
    if (!mesh.checkConnection() ) {
      //Serial.println("Link down");
      if (!mesh.renewAddress()) {
        //Serial.println("Unable to renew address, restarting..");
        mesh.begin(mesh_channel, rate, mesh_timeout);
      }
    }
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
    SendAck(sender, cmd.id, AckKey);
    return ;
  }
  if (cmd.crc == lastMsg) {
    SendAck(sender, cmd.id, AckRep);
    return ;
  }
  lastMsg = cmd.crc;
  onCommand(cmd);

  Serial.print(F("$cmd,"));
  Serial.print(cmd.sender);
  Serial.print(_comma);
  Serial.print(cmd.cmd);
  Serial.print(_comma);
  Serial.print(cmd.param);
  Serial.println(_end);

  SendAck(sender, cmd.id, AckOK);
}
