#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_break.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

CmdBreak::CmdBreak(CmdId id, cbuf_handle_t cbuf) : Cmd(id) {
  switch (id) {
    case BREAK_SET:
      parseBreakSet(cbuf);
      break;
    case BREAK_READ:
      if (!circular_buf_empty(cbuf)) {
        circular_buf_get(cbuf, &bpNo);
      } // TODO Else set error message
      break;
    case BREAKS:
      circular_buf_get(cbuf, &disableAll);
      break;
  }
}

void CmdBreak::parseBreakSet(cbuf_handle_t cbuf) {
  uint8_t tmpByte;

  while (!circular_buf_empty(cbuf)) {
    circular_buf_get(cbuf, &tmpByte);

    switch (tmpByte) {
      case BP_NUMBER: {
        circular_buf_get(cbuf, &bpNo);
      }  break;
      case BP_ENABLE:
        circular_buf_get(cbuf, &config.break_enabled[bpNo-1]);
        break;
      case BP_ADDRESS:
        circular_buf_get(cbuf, &config.break_addresses[bpNo-1].value_l);
        circular_buf_get(cbuf, &config.break_addresses[bpNo-1].value_h);
        circular_buf_get(cbuf, &config.break_addresses[bpNo-1].value_u);
        break;
      default:
        printf("CmdBreak::parseBreakSet: Field 0x%02X not found\n", tmpByte);
    }
  }
}

bool CmdBreak::execute() {
  outBuffer[0] = id; // Message type

  switch (id) {
    case BREAK_SET:
      setBreakpoint();
      // Following is the same as for BREAK_READ
    case BREAK_READ:
      outBuffer[1] = bpNo;
      outBuffer[2] = config.break_enabled[bpNo-1];
      outBuffer[3] = config.break_addresses[bpNo-1].value_l;
      outBuffer[4] = config.break_addresses[bpNo-1].value_h;
      outBuffer[5] = config.break_addresses[bpNo-1].value_u;
      break;
    case BREAKS: {
        if (disableAll) {
          config.break_enabled[0] = 0;
          config.break_enabled[1] = 0;
          config.break_enabled[2] = 0;
          config.break_enabled[3] = 0;

          // Set breakpoint state for all four breakpoints
          cmd_wr_buf[0] = 1 << 16 | PIO_WRITE_ZDI; // First two bytes are data size-1, next two bytes are operation type, read or write to ZDI
          cmd_wr_buf[1] = ZDI_BRK_CTL << 24 | getBrkCtlValue(false) << 16;

          dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
          dma_channel_set_trans_count(wr_dma_ch, 2, true);

          while (dma_channel_is_busy(wr_dma_ch))
            tight_loop_contents();
        }

        for (int i = 0; i <= 3; i++) {
          outBuffer[i * 5 + 1] = i + 1;
          outBuffer[i * 5 + 2] = config.break_enabled[i];
          outBuffer[i * 5 + 3] = config.break_addresses[i].value_l;
          outBuffer[i * 5 + 4] = config.break_addresses[i].value_h;
          outBuffer[i * 5 + 5] = config.break_addresses[i].value_u;
        }
      }
      break;
    case STEP:
      outBuffer[1] = singleStep();
      break;
  }

  return true;
}

ResponseBuf CmdBreak::getResponse() {
  ResponseBuf responseBuf;
  responseBuf.startAddr = Cmd::outBuffer;

  switch (id) {
    case BREAK_READ:
    case BREAK_SET:
      responseBuf.size = 6;
      break;
    case BREAKS:
      responseBuf.size = 21;
      break;
    case STEP:
      responseBuf.size = 2; // Command code and error code
      break;
  }

  return responseBuf;
}

void CmdBreak::setBreakpoint() {
  stopCPU();

  // Set addresses for all breakpoints
  cmd_wr_buf[0] = 12 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_ADDR0L << 24 | config.break_addresses[0].value_l << 16 | config.break_addresses[0].value_h << 8 | config.break_addresses[0].value_u; // Write address
  cmd_wr_buf[2] = config.break_addresses[1].value_l << 24 | config.break_addresses[1].value_h << 16 | config.break_addresses[1].value_u << 8 | config.break_addresses[2].value_l;
  cmd_wr_buf[3] = config.break_addresses[2].value_h << 24 | config.break_addresses[2].value_u << 16 | config.break_addresses[3].value_l << 8 | config.break_addresses[3].value_h;
  cmd_rd_buf[4] = config.break_addresses[3].value_u;

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, 5, true);

  while (dma_channel_is_busy(rd_dma_ch))
    tight_loop_contents();

  if (!config.ez80_stopped) {
    startCPU();
  }
}

uint8_t CmdBreak::singleStep() {
  if (!config.ez80_stopped) // CPU must be stopped to single step
    return ERR_STEP;

  // Single step
  cmd_wr_buf[0] = 1 << 16 | PIO_WRITE_ZDI; // First two bytes are data size-1, next two bytes are operation type, read or write to ZDI
  cmd_wr_buf[1] = ZDI_BRK_CTL << 24 | getBrkCtlValueWithSingleStep() << 16;

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, 2, true);

  return ERR_SUCCESS;
}