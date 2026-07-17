#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_runstop.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

CmdRunStop::CmdRunStop(CmdId id, cbuf_handle_t cbuf) : Cmd(id), isFullReset(0) {
  if (id == RESET) {
    circular_buf_get(cbuf, &isFullReset);
  }
}

bool CmdRunStop::execute() {
  switch (id) {
    case STOP:
      stopCPU();
      config.ez80_stopped = 1;
      break;
    case RUN:
      startCPU();
      config.ez80_stopped = 0;
      break;
    case RESET: {
        if (isFullReset)
          fullReset();
        else
          reset();
      }
      break;
  }

  return true;
}

void CmdRunStop::reset() {
  cmd_wr_buf[0] = 1 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_MASTER_CTL << 24 | ZDI_RESET << 16;

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start write address
  dma_channel_set_trans_count(wr_dma_ch, 2, true);
}

void CmdRunStop::fullReset() {
  // TODO Implement fullReset()
}