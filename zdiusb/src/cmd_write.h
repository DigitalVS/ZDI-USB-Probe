#ifndef CMD_WRITE_H
#define CMD_WRITE_H

#include "cmd.h"

class CmdWrite : public Cmd {
  public:
    CmdWrite(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;
  private:
    void calcChecksum();

    uint16_t dataSize;
};

#endif