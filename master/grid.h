#ifndef Grid_h
#define Grid_h

#include "Arduino.h"
#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"

#define mesh_pin_1 9
#define mesh_pin_2 10
#define mesh_channel 126 // 1-127
#define network_key 957241

enum {
  sTime = 1,              // real time clock
  sPir = 2,               // pir sensor
  sTemp = 3,              // temperature
  sHumid = 4,             // humidity
  sPress = 5,             // pressure
  sAc = 6,                // AC Voltage line in
  sLight = 7,             // Ambient light
  sAirDust = 8,           // Dust Sensor
  sGasO2 = 9,             // Oxygen Sensor
  sGasCO2 = 10,           // CO2 Sensor
  sGasETH = 11,           // Ethanol Sensor
  sVcc = 12,              // DC Voltage
  sLength = 13,           // Distance measurement in mm
  sState = 14,            // Application State

  sIR = 15,               // IR Remote signal
  sRF = 16,               // 443mhz remote signal
  sNFC = 17,              // NFC card signal
  sTcp = 18,              // Wifi,Gsm,Ethernet command

  sEEG = 18,              // Simplified EEG value
  sECG = 19,              // ECG Heart rate
  sWeight = 20,           // Weight scaler

  idCommand = 65,         // Secure command with key
  idAck = 66,             // Command ACK
  idPing = 67,            // Ping
  idTelemetry = 68,       // Single Sensor Value
  idTelemetrySet = 69,    // Multi Sensor Value

  AckOK = 1,    // command executed
  AckFail = 2,  // command received but cant processed
  AckKey = 3,   // key error
  AckRep = 4,   // repeated command
  AckSpf = 5    // Source address mismatch
};

struct __attribute__((packed)) Command {
  unsigned long id;     // command number
  uint16_t sender;      // sender address
  unsigned int cmd;     // command
  unsigned long param;  // parameter
  unsigned long crc;    // integrity check
};

struct __attribute__((packed)) Ack {
  uint16_t sender;
  unsigned int cmdId;
  unsigned int result;
};

struct __attribute__((packed)) Ping {
  unsigned long nanosec;
  unsigned long uptime;
  bool pong;
};

struct __attribute__((packed)) Pong {
  uint16_t sender;
  unsigned int ttl;
  unsigned long uptime;
};

struct __attribute__((packed)) Telemetry {
  uint16_t sender;
  unsigned int type;             // sensor type class
  unsigned long value;           // value
};

class GridNode {
  public:
    uint16_t _address;
    GridNode(uint16_t addr);
    boolean Command_(uint16_t address, unsigned int cmd, unsigned long param);
    boolean Telemetry_(uint16_t address, unsigned int type, unsigned long value);
    boolean Ping_(uint16_t address);
    void (*onCommand)(Command C);
    void (*onPong)(Pong p);
    void (*onAck)(Ack a);
    void (*onTelemetry)(Telemetry t);
    void Cycle();
    void flush();
  private:
    unsigned long count_ = 1;             // Command Count
    unsigned long _crc[4];                // Checksum buffer
    unsigned long lastCmd;                // Last received command
    unsigned long BuildCRC(Command cmd);
    void ProcessCommand(Command cmd, uint16_t sender);
    void Ack_(uint16_t address, unsigned int cid,  unsigned int result);
    void Receive();
};

#endif
