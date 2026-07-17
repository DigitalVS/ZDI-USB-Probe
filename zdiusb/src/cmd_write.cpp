#include <stdio.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "cmd_write.h"

extern Config config;

extern int wr_dma_ch;
extern int rd_dma_ch;

CmdWrite::CmdWrite(CmdId id, cbuf_handle_t cbuf) : Cmd(id) {
  circular_buf_get_range(cbuf, inBuffer, 3); // Address is in the beginning of the inBuffer
  circular_buf_get16(cbuf, &dataSize);

  uint8_t actualSize = circular_buf_get_range(cbuf, inBuffer + 3, dataSize);

  if (actualSize != dataSize) {
    printf("Actual data size is different then expected size (%d/%d)\n", actualSize, dataSize);
    actualSize = 0; // This indicates a read error!
    errCode = ERR_DATA_SIZE;
  }

  calcChecksum(); // Calculate checksum for received data
}

bool CmdWrite::execute() {
  stopCPU();

  if (!config.ez80_stopped)
    busy_wait_at_least_cycles(1000); // 27 x 32 = 864 clocks for 1MHz PIO speed

  cmd_wr_buf[0] = 4 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_WR_DATA_L << 24 | inBuffer[0] << 16 | inBuffer[1] << 8 | inBuffer[2]; // Write address (new PC register value)
  cmd_wr_buf[2] = REG_WR_PC << 24; // Write to PC register

  cmd_wr_buf[3] = dataSize << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[4] = ZDI_WR_MEM << 24 | inBuffer[3] << 16 | inBuffer[4] << 8 | inBuffer[5]; // 3 bytes put into the buffer regardless the dataSize value

  uint wrIndex = 5, inIndex = 6;
  int size = dataSize - 3; // Has to be signed data type for next checking if size > 0

  while (size > 0) {
    cmd_wr_buf[wrIndex++] = inBuffer[inIndex++] << 24 | inBuffer[inIndex++] << 16 | inBuffer[inIndex++] << 8 | inBuffer[inIndex++];
    size = size - 4;
  }

  // Next lines are basically the same as in Cmd::startCPU()
  if (!config.ez80_stopped) { // Restore PC register
    cmd_wr_buf[wrIndex++] = 4 << 16 | PIO_WRITE_ZDI; // Write
    cmd_wr_buf[wrIndex++] = ZDI_WR_DATA_L << 24 | config.pc_reg.value_l << 16 | config.pc_reg.value_h << 8 | config.pc_reg.value_u;
    cmd_wr_buf[wrIndex++] = REG_WR_PC << 24; // Write to PC register

    cmd_wr_buf[wrIndex++] = 1 << 16 | PIO_WRITE_ZDI; // Write
    cmd_wr_buf[wrIndex++] = ZDI_BRK_CTL << 24 | 0 << 16; // Clear Break on next instruction flag
  }

  dma_channel_set_read_addr(wr_dma_ch, cmd_wr_buf, false); // Reset start read address
  dma_channel_set_trans_count(wr_dma_ch, wrIndex, true);

  return true;
}

ResponseBuf CmdWrite::getResponse() {
  // Create a response message
  Cmd::outBuffer[0] = WRITE; // Message type
  Cmd::outBuffer[1] = checksum;

  return ResponseBuf { .startAddr = Cmd::outBuffer, .size = 2 };
}

void CmdWrite::calcChecksum() {
  uint64_t sum = 0;

  for (int i = 0; i < dataSize; i++) {
    sum += Cmd::inBuffer[i + 3];
  }

  checksum = (-sum) & 0xFF;
}
