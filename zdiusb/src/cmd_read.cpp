#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_read.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

CmdRead::CmdRead(CmdId id, cbuf_handle_t cbuf) : Cmd(id) {
  circular_buf_get_range(cbuf, inBuffer, 3); // Address is in the beginning of the inBuffer
  circular_buf_get16(cbuf, &dataSize);
  printf("dataSize: %d\n", dataSize);

  if (config.zdi_speed != ZDI_1MHz) // TODO Maybe this should be for all frequencies, including 1MHz! And down in code also! This could depend on speed of the memory.
    dataSize++; // Because first byte received will be garbage! This is happening because eZ80 needs some time to read byte from memory!
}

bool CmdRead::execute() {
  if (!Cmd::execute())
    return false;

  stopCPU();

  if (!config.ez80_stopped)
    busy_wait_at_least_cycles(1000);

  cmd_wr_buf[0] = 4 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_WR_DATA_L << 24 | inBuffer[0] << 16 | inBuffer[1] << 8 | inBuffer[2]; // Read address (new PC register value)
  cmd_wr_buf[2] = REG_WR_PC << 24; // Write to PC register

  cmd_wr_buf[3] = dataSize << 16 | PIO_READ_ZDI;
  cmd_wr_buf[4] = ZDI_RD_MEM << 24;

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, 5, true);

  while (dma_channel_is_busy(wr_dma_ch))
    tight_loop_contents();

  busy_wait_at_least_cycles(500);

  dma_channel_set_write_addr(rd_dma_ch, cmd_rd_buf, false); // DMA read
  dma_channel_set_trans_count(rd_dma_ch, dataSize + 1, true);

  while (dma_channel_is_busy(rd_dma_ch))
    tight_loop_contents();

  if (!config.ez80_stopped) {
    //busy_wait_at_least_cycles(500);

    startCPU();
  }

  return true;
}

ResponseBuf CmdRead::getResponse() {
  if (config.zdi_speed != ZDI_1MHz)
    dataSize--; // Because first byte received is garbage!

  // Create a response message
  Cmd::outBuffer[0] = READ; // Message type
  Cmd::outBuffer[1] = dataSize;

  for (int i = 1; i <= dataSize; i++) {
    Cmd::outBuffer[(i + 3) % 256] = cmd_rd_buf[config.zdi_speed == ZDI_1MHz ? i - 1 : i]; // Copy read bytes to output buffer (skip first byte if ZDI speed is more then 1MHz)
  }

  calcChecksum(); // Calculate checksum for received data
  Cmd::outBuffer[3] = checksum;

  ResponseBuf responseBuf;
  responseBuf.startAddr = Cmd::outBuffer;
  responseBuf.size = dataSize + 4; // Additional 4 is 1 for message type, 2 for size and 1 for checksum
  return responseBuf;
}

void CmdRead::calcChecksum() {
  uint64_t sum = 0;

  for (int i = 0; i < dataSize; i++) {
    sum += Cmd::outBuffer[i + 4];
  }

  checksum = (-sum) & 0xFF;
}