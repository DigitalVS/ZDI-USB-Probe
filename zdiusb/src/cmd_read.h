#ifndef CMD_READ_H
#define CMD_READ_H

#include "cmd.h"

class CmdRead : public Cmd {
  public:
    CmdRead(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;
  private:
    void calcChecksum();

    uint16_t dataSize;
};

#endif