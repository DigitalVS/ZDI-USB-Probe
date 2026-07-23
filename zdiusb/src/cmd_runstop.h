#ifndef CMD_RUNSTOP_H
#define CMD_RUNSTOP_H

#include "cmd.h"

class CmdRunStop : public Cmd {
  public:
    CmdRunStop(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;

  private:
    void reset(); // Send reset message over ZDI to reset only the CPU core
    void fullReset(); // Pull down reset signal

    uint8_t isFullReset;
    uint8_t stopOnReset;
};

#endif