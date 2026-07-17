#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_set.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

CmdSet::CmdSet(CmdId id, cbuf_handle_t cbuf) : Cmd(id), changeADL(false) {
  uint8_t tmpByte, tmp;

  while (!circular_buf_empty(cbuf)) {
    circular_buf_get(cbuf, &tmpByte);
    circular_buf_get(cbuf, &tmp);

    switch (tmpByte) {
      case ZDI_SPEED: {
        config.zdi_speed = (ZdiSpeed) (32 / tmp);
      }  break;
      case ADL_MODE:
        if (tmp != config.adl_mode) {
          config.adl_mode = tmp;
          changeADL = true;
        }
        break;
    }
  }
}

bool CmdSet::execute() {
  pio_sm_set_enabled(pioSm.pio, pioSm.sm, false);
  pio_sm_set_clkdiv_int_frac8 (pioSm.pio, pioSm.sm, config.zdi_speed, 0);
  pio_sm_set_enabled(pioSm.pio, pioSm.sm, true);

  if (changeADL) {
    uint index = 0;

    if (!config.ez80_stopped) { // If not in break state, break on next instruction
      cmd_wr_buf[index++] = 1 << 16 | PIO_WRITE_ZDI; // Write
      cmd_wr_buf[index++] = ZDI_BRK_CTL << 24 | ZDI_BRK_NEXT << 16;
    }

    // Set ADL mode value
    cmd_wr_buf[index++] = 1 << 16 | PIO_WRITE_ZDI; // Write
    cmd_wr_buf[index++] = ZDI_RW_CTL << 24 | (config.adl_mode == 1 ? ADL_SET : ADL_CLR) << 16;

    dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start write address
    dma_channel_set_trans_count(wr_dma_ch, index, true);

    changeADL = false;

    while (dma_channel_is_busy(wr_dma_ch))
      tight_loop_contents();

    if (!config.ez80_stopped) { // Restore runing state
      busy_wait_at_least_cycles(500);

      cmd_wr_buf[0] = 1 << 16 | PIO_WRITE_ZDI; // Write
      cmd_wr_buf[1] = ZDI_BRK_CTL << 24 | 0 << 16; // Clear Break on next instruction flag

      dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start write address
      dma_channel_set_trans_count(wr_dma_ch, 2, true);
    }
  }

  return true;
}