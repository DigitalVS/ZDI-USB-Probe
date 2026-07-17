#ifndef CMD_BREAK_H
#define CMD_BREAK_H

#include "cmd.h"

typedef enum : uint8_t {BP_NUMBER, BP_ENABLE, BP_ADDRESS} BreakpointFields;

class CmdBreak : public Cmd {
  public:
    CmdBreak(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;
  private:
    void parseBreakSet(cbuf_handle_t cbuf);
    void setBreakpoint();
    uint8_t singleStep(); // Returns error code

    uint8_t bpNo;
    uint8_t disableAll;
};

#endif