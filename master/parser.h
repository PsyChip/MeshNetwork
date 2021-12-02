#ifndef Parser_h
#define Parser_h

#include "Arduino.h"

#define cmdLen 3

// command format: [command:param<newline char(13)>]
// case sensitive

const String Commands[cmdLen] = {
  "cmd", "ping", "list"
};

class Parser {
  public:
    Parser();
    int paramInt();
    void splitParamInt();
    int Poll();
    void (*onCommand)(int CmdId);
    void flush();
    String buffer;
    String command;
    String param;
    int paramsInt[6];
  private:
    unsigned long lastcmd = 0;
    const unsigned long timeout = 2000;
    int GetCmdId();
    bool Receive();
    bool parse();
};

#endif
