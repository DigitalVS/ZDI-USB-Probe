#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_status.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

uint8_t fifo_get_byte();

CmdStatus::CmdStatus(CmdId id, cbuf_handle_t cbuf) : Cmd(id) {
}

bool CmdStatus::execute() {
  if (!Cmd::execute())
    return false;

  // Get CPU state from the target
  pioSm.pio->txf[pioSm.sm] = 0x0000 << 16 | PIO_READ_ZDI; // Read
  pioSm.pio->txf[pioSm.sm] = ZDI_STAT << 24;
  // Receive 1 byte
  config.zdi_status = fifo_get_byte();
  config.ez80_stopped = (config.zdi_status >> 7) & 1; // Value is 1 if ZDI mode is active
  config.adl_mode = (config.zdi_status >> 4) & 1;
  return true;
}

ResponseBuf CmdStatus::getResponse() {
  // Create a response message
  Cmd::outBuffer[0] = STATUS; // Message type
  Cmd::outBuffer[1] = ZDI_SPEED;
  Cmd::outBuffer[2] = 32 /config.zdi_speed;
  Cmd::outBuffer[3] = ZDI_STATUS;
  Cmd::outBuffer[4] = config.zdi_status;

  return ResponseBuf { .startAddr = Cmd::outBuffer, .size = 5 };
}