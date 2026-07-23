#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/time.h"
#include "cmd_runstop.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

CmdRunStop::CmdRunStop(CmdId id, cbuf_handle_t cbuf) : Cmd(id), isFullReset(0), stopOnReset(0) {
  if (id == RESET) {
    circular_buf_get(cbuf, &isFullReset);
    circular_buf_get(cbuf, &stopOnReset);
  }
}

bool CmdRunStop::execute() {
  if (!Cmd::execute())
    return false;

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
  pio_sm_set_enabled(pioSm.pio, pioSm.sm, false); // Disable PIO state machine

  gpio_put(RESET_GPIO, 1);

  if (stopOnReset) // Set ZDA pin to low. Set ZCL pin to high if stopOnReset, otherwise low
    pio_sm_exec(pioSm.pio, pioSm.sm, pio_encode_set(pio_pins, 0b10)); // Note that first pin (ZDA) is one with the lowest value (0)!
  else
    pio_sm_exec(pioSm.pio, pioSm.sm, pio_encode_set(pio_pins, 0b11)); // Both ZDA and ZCL are high, the same state as pull-up resistors would set on their own

  sleep_ms(15); // Active reset time
  gpio_put(RESET_GPIO, 0);

  if (stopOnReset) { // Change ZDA/ZCL pin combination to non-blocking
    sleep_ms(1);
    pio_sm_exec(pioSm.pio, pioSm.sm, pio_encode_set(pio_pins, 0b11));
  }

  pio_sm_set_enabled(pioSm.pio, pioSm.sm, true); // Enable PIO state machine

  if (!stopOnReset) {
    reset(); // Reset only the CPU again because it is somehow blocked after the reset even if ZDA and ZCL pin state does not block it
  }
}