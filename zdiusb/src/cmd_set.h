#ifndef CMD_SET_H
#define CMD_SET_H

#include "cmd.h"

class CmdSet : public Cmd {
  public:
    CmdSet(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;

  private:
    bool changeADL;
};

#endif