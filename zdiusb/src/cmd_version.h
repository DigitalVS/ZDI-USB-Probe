#ifndef CMD_VERSION_H
#define CMD_VERSION_H

#include "cmd.h"

class CmdVersion : public Cmd {
  public:
    CmdVersion(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;

  private:

};

#endif