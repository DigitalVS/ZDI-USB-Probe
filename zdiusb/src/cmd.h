#ifndef CMD_H
#define CMD_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware.h"
#include "circular_buffer.h"
#include "zdi.h"

const uint16_t CMD_DATA_BUF_SIZE = 4096;
const uint16_t DMA_DATA_BUF_SIZE = 4196; // 100 bytes is for overhead data (save and restore PC register, etc.)

typedef enum : uint8_t {ERR_SUCCESS, ERR_UNKNOWN, ERR_PARAM, ERR_DATA_SIZE, ERR_TIMEOUT, ERR_CHECKSUM, ERR_KEY_INTERRUPT, ERR_STEP, ERR_DISASSEMBLER} CmdErrorCode;
typedef enum : uint8_t {ERROR, VERSION, STATUS, SET, GET, READ, WRITE, VERIFY, BREAK_SET, BREAK_READ, BREAKS, STEP, REG_SET, REG_READ, REGS, REGS_EXX, RUN, STOP,
  RESET, DISASSEMBLE, DISASSEMBLE_ADDR} CmdId;
typedef enum : uint8_t {ZDI_SPEED = 1, ADL_MODE, ZDI_STATUS, TARGET_CONNECTED, BOOT_MODE} StateType; // Used by SET and STATE commands

typedef enum : uint8_t {BP_DISABLED, BP_ENABLED, BP_NOT_SET} BreakpointStatus;

typedef struct {
  void* startAddr;
  uint32_t size;
} ResponseBuf;

class Cmd {
  public:
    Cmd(CmdId id);

    static Cmd* create(cbuf_handle_t cbuf);

    virtual bool execute() = 0;
    virtual ResponseBuf getResponse() { return ResponseBuf { .startAddr = NULL, .size = 0 }; }
    CmdId getId() { return id; }

    static inline uint32_t cmd_wr_buf[DMA_DATA_BUF_SIZE / 4];
    static inline uint8_t cmd_rd_buf[DMA_DATA_BUF_SIZE];
  protected:
    uint8_t getBrkCtlValue(const bool break_next);
    uint8_t getBrkCtlValueWithSingleStep();
    void stopCPU();
    void startCPU();

    static inline uint8_t checksum = 0;
    CmdId id;
    CmdErrorCode errCode;

    static inline uint8_t inBuffer[CMD_DATA_BUF_SIZE]; // Buffers are public so that DMA configuration from main() can access them
    static inline uint8_t outBuffer[CMD_DATA_BUF_SIZE];
  private:
    uint8_t getBrkCtlValue(const bool break_next, const bool single_step);
};

extern PioSm pioSm;

inline uint8_t fifo_get_byte() {
  // 8-bit read from the uppermost byte of the FIFO, as data is left-justified
  io_rw_8 *rx_fifo_shift = (io_rw_8*) &pioSm.pio->rxf[pioSm.sm];

  while (pio_sm_is_rx_fifo_empty(pioSm.pio, pioSm.sm))
    tight_loop_contents();

  return (uint8_t)*rx_fifo_shift;
}

#endif