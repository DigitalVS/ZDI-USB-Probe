#ifndef CMD_REG_H
#define CMD_REG_H

#include "cmd.h"

typedef enum : uint8_t {REG_A, REG_F, REG_AF, REG_B, REG_C, REG_BC, REG_D, REG_E, REG_DE, REG_H, REG_L, REG_HL, REG_IXH, REG_IXL, REG_IX,
   REG_IYH, REG_IYL, REG_IY, REG_SP, REG_PC} Registers;

class CmdReg : public Cmd {
  public:
    CmdReg(CmdId id, cbuf_handle_t cbuf);
    bool execute() override;
    ResponseBuf getResponse() override;
  private:
    void setReg();
    Uint24 readReg(uint8_t regId);
    Uint24 readReg();
    void readRegs();
    uint8_t getRegCtl();

    uint8_t reg;
    uint8_t isLong; // Long option state for 24-bit write for register pair in non-ADL mode. Ignored in other cases.
};

#endif