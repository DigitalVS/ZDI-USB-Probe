#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_reg.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

static uint8_t regList[] = {ZDI_REG_AF, ZDI_REG_BC, ZDI_REG_DE, ZDI_REG_HL, ZDI_REG_IX, ZDI_REG_IY, ZDI_REG_SP, ZDI_REG_PC};
static uint8_t regPairs[] = {REG_AF, REG_BC, REG_DE, REG_HL, REG_IX, REG_IY, REG_PC, REG_SP};

CmdReg::CmdReg(CmdId id, cbuf_handle_t cbuf) : Cmd(id), isLong(0) {
  switch (id) {
    case REG_SET:
      if (!circular_buf_empty(cbuf)) {
        circular_buf_get(cbuf, &reg);

        circular_buf_get(cbuf, &inBuffer[0]);
        circular_buf_get(cbuf, &inBuffer[1]);
        circular_buf_get(cbuf, &inBuffer[2]);
        circular_buf_get(cbuf, &isLong);

        //printf("%02x %02x %02x\n", inBuffer[0], inBuffer[1], inBuffer[2]);
      } // TODO Else set error message
      break;
    case REG_READ:
      if (!circular_buf_empty(cbuf)) {
        circular_buf_get(cbuf, &reg);
      } // TODO Else set error message
      break;
  }

  //printf("%02x\n", reg);
}

bool CmdReg::execute() {
  outBuffer[0] = id; // Message type

  switch (id) {
    case REG_SET: {
        setReg();

        outBuffer[1] = reg;
        outBuffer[2] = inBuffer[0]; // Return the same value as received
        outBuffer[3] = inBuffer[1];
        outBuffer[4] = inBuffer[2];
      }
      break;
    case REG_READ: {
        stopCPU();

        Uint24 regValue = readReg();

        if (!config.ez80_stopped)
          startCPU();

        outBuffer[1] = reg;
        outBuffer[2] = regValue.value_l;
        outBuffer[3] = regValue.value_h;
        outBuffer[4] = regValue.value_u;
      }
      break;
    case REGS:
      // Register values are stored directly to the outBuffer
      readRegs();
      break;
    case REGS_EXX: {
        stopCPU();
        readReg(REG_EX);

        if (!config.ez80_stopped)
          startCPU();
      }
      break;
  }

  return true;
}

ResponseBuf CmdReg::getResponse() {
  ResponseBuf responseBuf;
  responseBuf.startAddr = outBuffer;
  responseBuf.size = id == REGS ? 25 : 5;
  return responseBuf;
}

void CmdReg::setReg() {
  stopCPU();

  Uint24 regValue(inBuffer[0], inBuffer[1], inBuffer[2]);
  // First check if register to change is 8-bit long
  bool regPair = false;

  for (int i = 0; i < sizeof(regPairs); i++) // Check if register is 24 bit or it is less?
    if (reg == regPairs[i]) { // Or 0x7f will mask out last bit which is set for write operation
      regPair = true;
      break;
    }

  if (!regPair) { // Register is one byte long, so read first its value to find rest of the bytes
    Uint24 regReadValue = readReg(getRegCtl() & 0x7F);

    // Leave lowest byte in inBuffer[0] unchanged
    regValue.value_h = regReadValue.value_h;
    regValue.value_u = regReadValue.value_u;
  } else if (!config.adl_mode && !isLong) { // Register pairs in 16-bit mode
    Uint24 regReadValue = readReg(getRegCtl() & 0x7F);

    regValue.value_u = regReadValue.value_u; // Keep the highest byte
  }

  // Write new register value
  cmd_wr_buf[0] = 3 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_WR_DATA_L << 24 | regValue.value_l << 16 | regValue.value_h << 8 | regValue.value_u; // New register value

  cmd_wr_buf[2] = 1 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[3] = ZDI_RW_CTL << 24 | getRegCtl() << 16; // Write register

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, 4, true);

  while (dma_channel_is_busy(wr_dma_ch))
    tight_loop_contents();

  if (!config.ez80_stopped) {
    startCPU();
  }
}

Uint24 CmdReg::readReg() {
  return readReg(getRegCtl());
}

Uint24 CmdReg::readReg(uint8_t regId) {
  Uint24 regValue;

  cmd_wr_buf[0] = 1 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_RW_CTL << 24 | regId << 16; // Read register

  cmd_wr_buf[2] = 2 << 16 | PIO_READ_ZDI; // Read
  cmd_wr_buf[3] = ZDI_RD_DATA_L << 24; // ZDI address

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, 4, true);

  while (dma_channel_is_busy(wr_dma_ch))
    tight_loop_contents();

  busy_wait_at_least_cycles(500);

  dma_channel_set_write_addr(rd_dma_ch, &regValue, false); // Reset start write address
  dma_channel_set_trans_count(rd_dma_ch, 3, true);

  while (dma_channel_is_busy(rd_dma_ch))
    tight_loop_contents();

  return regValue;
}

void CmdReg::readRegs() {
  stopCPU();

  for (int i = 0; i < sizeof(regList); i++) { // 8 registers with 3 bytes each
    Uint24 regValue = readReg(regList[i]);

    outBuffer[i * 3 + 1] = regValue.value_l;
    outBuffer[i * 3 + 2] = regValue.value_h;
    outBuffer[i * 3 + 3] = regValue.value_u;
  }

  if (!config.ez80_stopped) {
    startCPU();
  }
}

uint8_t CmdReg::getRegCtl() {
  uint8_t regCtl = 255;

  switch (reg) {
    case REG_A:
    case REG_F:
    case REG_AF:
      regCtl = ZDI_REG_AF;
      break;
    case REG_B:
    case REG_C:
    case REG_BC:
      regCtl = ZDI_REG_BC;
      break;
    case REG_D:
    case REG_E:
    case REG_DE:
      regCtl = ZDI_REG_DE;
      break;
    case REG_H:
    case REG_L:
    case REG_HL:
      regCtl = ZDI_REG_HL;
      break;
    case REG_IXH:
    case REG_IXL:
    case REG_IX:
      regCtl = ZDI_REG_IX;
      break;
    case REG_IYH:
    case REG_IYL:
    case REG_IY:
      regCtl = ZDI_REG_IY;
      break;
    case REG_SP:
      regCtl = ZDI_REG_SP;
      break;
    case REG_PC:
      regCtl = ZDI_REG_PC;
      break;
    default:
      printf("CmdReg::getRegCtl(): Unknown register id: %d\n", reg);
      return 255;
  }

  if (id == REG_SET) // Register write operation
    regCtl |= ZDI_REG_WR;

  return regCtl;
}