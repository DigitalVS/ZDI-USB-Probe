#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "cmd.h"
#include "cmd_version.h"
#include "cmd_set.h"
#include "cmd_status.h"
#include "cmd_read.h"
#include "cmd_write.h"
#include "cmd_break.h"
#include "cmd_reg.h"
#include "cmd_runstop.h"
#include "cmd_disassemble.h"

extern int wr_dma_ch;
extern int rd_dma_ch;

const float ADC_CONST = 3.3f / 4096.0f; // 3.3V is a reference voltage

Config config = {
  .ez80_stopped = 0,
  .adl_mode = 1,
  .zdi_speed = ZDI_1MHz,
  .target_connected = 0,
  .break_enabled = { BP_NOT_SET, BP_NOT_SET, BP_NOT_SET, BP_NOT_SET }
};

void wr_dma_irq_handler() {
  dma_hw->ints0 = 1u << wr_dma_ch; // Clear the interrupt request
  //dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address

  //printf("wr_dma_irq_handler()\n");
}

void rd_dma_irq_handler() {
  dma_hw->ints1 = 1u << rd_dma_ch; // Clear the interrupt request
  //dma_channel_set_write_addr(rd_dma_ch, cmd_rd_buf, false); // Reset start write address

  //printf("rd_dma_irq_handler()\n");
}

//-------------------------------------

Cmd::Cmd(CmdId id) : id(id), errCode(ERR_SUCCESS) {
}

Cmd* Cmd::create(cbuf_handle_t cbuf) {
  uint8_t cmdCode;
  circular_buf_get(cbuf, &cmdCode);

  //printf("Received cmd: %x\n", cmdCode);

  switch (cmdCode) {
    case STATUS:
      return new CmdStatus((CmdId) cmdCode, cbuf);
    case SET: {
      return new CmdSet((CmdId) cmdCode, cbuf);
    case WRITE:
      return new CmdWrite((CmdId) cmdCode, cbuf);
    case READ:
      return new CmdRead((CmdId) cmdCode, cbuf);
    case BREAK_SET:
    case BREAK_READ:
    case BREAKS:
    case STEP:
      return new CmdBreak((CmdId) cmdCode, cbuf);
    case REG_SET:
    case REG_READ:
    case REGS:
    case REGS_EXX:
      return new CmdReg((CmdId) cmdCode, cbuf);
    case RUN:
    case STOP:
    case RESET:
      return new CmdRunStop((CmdId) cmdCode, cbuf);
    case DISASSEMBLE:
    case DISASSEMBLE_ADDR:
      return new CmdDisassemble((CmdId) cmdCode, cbuf);
    case VERSION:
      return new CmdVersion((CmdId) cmdCode, cbuf);
    default:
      printf("Cmd::create(): cmdCode %d not found\n", cmdCode);
    }
  }

  return NULL;
}

bool Cmd::execute() {
  uint16_t raw_value = adc_read();
  float voltage = raw_value * ADC_CONST; // Voltage is halved with divider, so max voltage in normal circumstances is about 1.65V

  if (voltage < 0.9f) { // If actual voltage is less than 1.8V, target device is considered as not connected
    errCode = ERR_NO_TARGET;
    return false;
  }

  return true;
}

ResponseBuf Cmd::getErrResponse() {
  Cmd::outBuffer[0] = ERROR; // Message type
  Cmd::outBuffer[1] = errCode;
  return ResponseBuf { .startAddr = Cmd::outBuffer, .size = 2 };
}

uint8_t Cmd::getBrkCtlValue(const bool break_next, const bool single_step) {
  uint8_t brkCtl = 0;

  if (break_next) brkCtl |= ZDI_BRK_NEXT;
  if (config.break_enabled[3]) brkCtl |= ZDI_BRK_ADDR3;
  if (config.break_enabled[2]) brkCtl |= ZDI_BRK_ADDR2;
  if (config.break_enabled[1]) brkCtl |= ZDI_BRK_ADDR1;
  if (config.break_enabled[0]) brkCtl |= ZDI_BRK_ADDR0;
  if (single_step) brkCtl |= ZDI_SINGLE_STEP;

  return brkCtl;
}

uint8_t Cmd::getBrkCtlValueWithSingleStep() {
  return getBrkCtlValue(true, true);
}

uint8_t Cmd::getBrkCtlValue(const bool break_next) {
  return getBrkCtlValue(break_next, false);
}

void Cmd::stopCPU() {
  if (!config.ez80_stopped) { // If CPU is not yet stopped
    // Break on next instruction
    cmd_wr_buf[0] = 1 << 16 | PIO_WRITE_ZDI; // First two bytes are data size-1, next two bytes are operation type, read or write to ZDI
    cmd_wr_buf[1] = ZDI_BRK_CTL << 24 | getBrkCtlValue(true) << 16;

    // Save PC register value
    cmd_wr_buf[2] = 1 << 16 | PIO_WRITE_ZDI; // Write
    cmd_wr_buf[3] = ZDI_RW_CTL << 24 | REG_RD_PC << 16; // Read PC register

    cmd_wr_buf[4] = 2 << 16 | PIO_READ_ZDI; // Read
    cmd_wr_buf[5] = ZDI_RD_DATA_L << 24; // ZDI address

    dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
    dma_channel_set_trans_count(wr_dma_ch, 6, true);

    while (dma_channel_is_busy(wr_dma_ch))
      tight_loop_contents();

    busy_wait_at_least_cycles(500); // 27 x 32 = 864 clocks for 1MHz PIO speed

    dma_channel_set_write_addr(rd_dma_ch, &config.pc_reg, false); // Reset start write address
    dma_channel_set_trans_count(rd_dma_ch, 3, true);

    while (dma_channel_is_busy(rd_dma_ch))
      tight_loop_contents();
  }
}

void Cmd::startCPU() {
  // Restore PC register
  cmd_wr_buf[0] = 4 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_WR_DATA_L << 24 | config.pc_reg.value_l << 16 | config.pc_reg.value_h << 8 | config.pc_reg.value_u;
  cmd_wr_buf[2] = REG_WR_PC << 24; // Write to PC register

  cmd_wr_buf[3] = 1 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[4] = ZDI_BRK_CTL << 24 | getBrkCtlValue(false) << 16; // Clear Break on next instruction flag

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, 5, true);
}