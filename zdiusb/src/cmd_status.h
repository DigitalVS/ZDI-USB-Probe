#ifndef CMD_STATUS_H
#define CMD_STATUS_H

#include "cmd.h"

class CmdStatus : public Cmd {
  public:
    CmdStatus(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;

  private:

};

#endif