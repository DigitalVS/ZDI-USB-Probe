#ifndef CMD_DISASSEMBLE_H
#define CMD_DISASSEMBLE_H

#include "cmd.h"

class CmdDisassemble : public Cmd {
  public:
    CmdDisassemble(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;
  private:
    void readData(); // Read data from the target device

    uint16_t respLength; // Response message contents length
    uint16_t instNo; // Number of instructions in a message
    Uint24 startAddr;
    uint8_t dataSize; // Data length to read from the target
    uint8_t flags; // Options bit field
};

#endif