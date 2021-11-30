#ifndef Grid_h
#define Grid_h

#include "Arduino.h"
#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

#define mesh_pin_1 9
#define mesh_pin_2 10
#define mesh_timeout 3000
#define mesh_channel 126 // 1-127
#define mesh_conn_check 3000
#define network_key 957241

enum {
  sTime = 1,    // real time clock
  sPir = 2,     // pir sensor
  sTemp = 3,    // temperature
  sHumid = 4,   // humidity
  sPress = 5,   // pressure
  sAc = 6,      // AC Voltage line in
  sLight = 7,   // Ambient light
  sAirDust = 8, // Dust Sensor
  sGasO2 = 9,   // Oxygen Sensor
  sGasCO2 = 10, // CO2 Sensor
  sGasETH = 11, // Ethanol Sensor
  sVcc = 12,    // DC Voltage
  sLength = 13, // Distance measurement in mm
  sState = 14,  // Application State
  //////////////// Remote Retrofitting
  sIR = 15,     // IR Remote signal
  sRF = 16,     // 443mhz remote signal
  sNFC = 17,    // NFC card signal
  sTcp = 18,    // Wifi,Gsm,Ethernet command
  //////////////// bioelectronics
  sEEG = 18,    // Simplified EEG value
  sECG = 19,    // ECG Heart rate
  sWeight = 20, // Weight scaler

  // payload types
  idCommand = 1,
  idAck = 2,
  idPing = 3,
  idSensor = 4,
  idApp = 5,

  // command results
  AckOK = 1,    // command executed
  AckFail = 2,  // command received but cant processed
  AckKey = 3,   // key error
  AckRep = 4,   // repeated command
};

struct Command {
  unsigned int id;      // command number
  unsigned int sender;  // sender id
  unsigned int cmd;     // command
  unsigned long param;  // parameter
  unsigned long crc;    // integrity check
};

struct Ack {
  unsigned int sender;
  unsigned int cmdId;
  unsigned int result;
};

struct Ping {
  unsigned int nanosec;
  unsigned long uptime;
  bool pong;
};

struct Sensor {
  int type;             // sensor type class
  unsigned long value;  // value
};

class GridNode {
  public:
    unsigned int nodeID;
    GridNode(unsigned int nodeId);
    void Cycle(unsigned long now);
    int SendCommand(int node, int cmd, unsigned long param);
    void SendValue(uint16_t address, int type, int value);
    void (*onCommand)(Command C);
    void SendPing(uint8_t _nodeId);
    void List();
  private:
    unsigned long timer[2];
    unsigned long CommandNr = 1;
    unsigned long BuildCRC(Command cmd);
    unsigned long _crc[4];
    unsigned long lastMsg;
    void Receive();
    void SendPingEx(uint16_t address);
    void SendAck(uint16_t addr, int cid, int result);
    void ProcessCommand(Command cmd, uint16_t sender);
    void CheckConnection(unsigned long now);
    void Broadcast(unsigned long now);
    uint8_t AddrToId(uint16_t addr);
    uint16_t IdToAddr(uint8_t nodeId);
};

#endif
