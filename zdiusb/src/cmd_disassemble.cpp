#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "zdis.h"
#include "cmd_disassemble.h"

extern int wr_dma_ch;
extern int rd_dma_ch;

extern Config config;

#define DA_BUF_SIZE 30

static uint8_t da_buf[DA_BUF_SIZE]; // One line disassembler buffer
static uint8_t* da_buf_index; // Current buffer byte

enum Options {
  SUFFIXED_IMM = 1 << 0, // 1 - Intel hex, 0 - Motorola hex representation (with $ prefix)
  DECIMAL_IMM = 1 << 1, // 1 - decimal number, 0 - hex number
  MNE_SPACE = 1 << 2, // 1 - space after mnemonic , 0 - tab after mnemonic
  ARG_SPACE = 1 << 3, // 1 - space between arguments, 0 - without space after ','
  COMPUTE_REL = 1 << 4,
  COMPUTE_ABS = 1 << 5,
};

static int read(struct zdis_ctx *ctx, uint32_t addr) {
  return *((uint8_t*) (ctx->zdis_user_ptr + addr));
}

static bool put(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il) {
  char pattern[8], *p = pattern;
  switch (kind) {
  case ZDIS_PUT_REL: // JR/DJNZ targets
    val += ctx->zdis_end_addr;
    if (ctx->zdis_user_size & COMPUTE_REL) {
      return put(ctx, ZDIS_PUT_WORD, val, il);
    }

    *da_buf_index++ = '$';
    if (da_buf_index <= da_buf + DA_BUF_SIZE) {
      return false;
    }

    val -= ctx->zdis_start_addr;
    // fallthrough
  case ZDIS_PUT_OFF: // immediate offsets from index registers
    if (val > 0) {
      *p++ = '+';
    } else if (val < 0) {
      *p++ = '-';
      val = -val;
    } else {
      return true;
    }
    // fallthrough
  case ZDIS_PUT_BYTE: // byte immediates
  case ZDIS_PUT_PORT: // immediate ports
  case ZDIS_PUT_RST: { // RST targets
    if (ctx->zdis_user_size & DECIMAL_IMM) {
      *p++ = '%';
      *p++ = 'u';
      if (ctx->zdis_user_size & SUFFIXED_IMM) {
        *p++ = 'd';
      }
    } else {
      *p++ = ctx->zdis_user_size & SUFFIXED_IMM ? '0' : '$';
      *p++ = '%';
      *p++ = '0';
      *p++ = '2';
      *p++ = ctx->zdis_lowercase ? 'x' : 'X';
      if (ctx->zdis_user_size & SUFFIXED_IMM) {
        *p++ = 'h';
      }
    }
    *p = '\0';
    int len = sprintf((char*) da_buf_index, pattern, val);
    da_buf_index += len;
    return len > 0;
  }
  case ZDIS_PUT_ABS: // JP/CALL immediate targets
    if (ctx->zdis_user_size & COMPUTE_ABS) {
      int32_t extend = il ? 8 : 16;
      *da_buf_index++ = '$';
      return da_buf_index <= da_buf + DA_BUF_SIZE && put(ctx, ZDIS_PUT_OFF, (int32_t)(val - ctx->zdis_start_addr) << extend >> extend, il);
    }
    // fallthrough
  case ZDIS_PUT_WORD: // word immediates (il ? 24 : 16) bits wide
  case ZDIS_PUT_ADDR: { // load/store immediate addresses
    if (ctx->zdis_user_size & DECIMAL_IMM) {
      *p++ = '%';
      *p++ = 'u';
      if (ctx->zdis_user_size & SUFFIXED_IMM) {
        *p++ = 'd';
      }
    } else {
      *p++ = ctx->zdis_user_size & SUFFIXED_IMM ? '0' : '$';
      *p++ = '%';
      *p++ = '0';
      *p++ = il ? '6' : '4';
      *p++ = ctx->zdis_lowercase ? 'x' : 'X';
      if (ctx->zdis_user_size & SUFFIXED_IMM) {
        *p++ = 'h';
      }
    }
    *p = '\0';
    int len = sprintf((char*) da_buf_index, pattern, val);
    da_buf_index += len;
    return len > 0;
  }
  case ZDIS_PUT_CHAR: // one character of mnemonic, register, or parentheses
    *da_buf_index++ = (uint8_t) val;
    return da_buf_index <= da_buf + DA_BUF_SIZE;
  case ZDIS_PUT_MNE_SEP: // between mnemonic and arguments
    *da_buf_index++ = ctx->zdis_user_size & MNE_SPACE ? ' ' : '\t';
    return da_buf_index <= da_buf + DA_BUF_SIZE;
  case ZDIS_PUT_ARG_SEP: // between two arguments
    *da_buf_index++ = ',';
    if (ctx->zdis_user_size & ARG_SPACE) {
      *da_buf_index++ = ' ';
    }
    return da_buf_index <= da_buf + DA_BUF_SIZE;
  case ZDIS_PUT_END: // at end of instruction
    //*da_buf_index++ = '\0';
    return true;
  }
  // return false for error
  return false;
}

//-------------------------------------

CmdDisassemble::CmdDisassemble(CmdId id, cbuf_handle_t cbuf) : Cmd(id), instNo(0), respLength(0), dataSize(64), flags(0) {
  switch (id) {
    case DISASSEMBLE:
      startAddr = config.pc_reg;
      break;
    case DISASSEMBLE_ADDR: {
      circular_buf_get(cbuf, &startAddr.value_l);
      circular_buf_get(cbuf, &startAddr.value_h);
      circular_buf_get(cbuf, &startAddr.value_u);
    }  break;
  }

  circular_buf_get(cbuf, (uint8_t*) &flags);
  circular_buf_get(cbuf, (uint8_t*) &dataSize);
}

bool CmdDisassemble::execute() {
  if (id == DISASSEMBLE && !config.ez80_stopped) { // For disassembling from PC address, if CPU not stopped, do not execute the command
    return true;
  }

  readData();

  struct zdis_ctx ctx = {
    .zdis_read = read,      // callback for getting bytes to disassemble
    .zdis_put = put,        // callback for processing disassembly output
    .zdis_start_addr = 0,   // starting address of the current instruction
    .zdis_end_addr = 0,     // ending address of the current instruction
    .zdis_lowercase = true, // automatically convert ZDIS_PUT_CHAR characters to lowercase
    .zdis_implicit = true,  // omit certain destination arguments as per z80 style assembly
    .zdis_adl = config.adl_mode ? true : false, // default word width when not overridden by suffix
    .zdis_user_ptr = cmd_rd_buf, // arbitrary use
    .zdis_user_size = flags // arbitrary use
  };

  da_buf_index = da_buf;
  uint32_t start_addr = startAddr.value_u << 16 | startAddr.value_h << 8 | startAddr.value_l;
  uint32_t index = 5; // Start instructions outBuffer index (first five bytes are put in getResponse() function)

  while (ctx.zdis_end_addr < dataSize && zdis_put_inst(&ctx)) {
    uint32_t instrAddr = start_addr + ctx.zdis_start_addr;

    // Three byte address field, little endian
    Cmd::outBuffer[index++] = instrAddr & 0xFF;
    Cmd::outBuffer[index++] = (instrAddr >> 8) & 0xFF;
    Cmd::outBuffer[index++] = (instrAddr >> 16) & 0xFF;

    // Length of string is da_buf_index - da_buf
    uint16_t instStrLength = da_buf_index - da_buf;
    Cmd::outBuffer[index++] = instStrLength & 0xFF;
    Cmd::outBuffer[index++] = (instStrLength >> 8) & 0xFF;

    for (int i = 0; i < instStrLength; i++)
      Cmd::outBuffer[index++] = da_buf[i];

    instNo++;
    respLength += 5 + instStrLength; // 5 - three byte address + two byte instruction string length
    da_buf_index = da_buf;
  }

  return true;
}

ResponseBuf CmdDisassemble::getResponse() {
  if (id == DISASSEMBLE && !config.ez80_stopped) { // For disassembling from PC address, CPU must be in stopped state
    Cmd::outBuffer[0] = ERROR; // Message type
    Cmd::outBuffer[1] = ERR_DISASSEMBLER; // Error code
    return ResponseBuf { .startAddr = Cmd::outBuffer, .size = 2};
  }

  respLength += 2; // Two additional bytes for instNo

  // Create a response message
  Cmd::outBuffer[0] = id; // Message type
  Cmd::outBuffer[1] = respLength & 0xFF; // Two bytes length
  Cmd::outBuffer[2] = (respLength >> 8) & 0xFF;
  Cmd::outBuffer[3] = instNo & 0xFF; // Two bytes number of instructions in a message
  Cmd::outBuffer[4] = (instNo >> 8) & 0xFF;

  return ResponseBuf { .startAddr = Cmd::outBuffer, .size = (uint32_t) respLength + 3}; // Additional three bytes for cmd name and length
}

void CmdDisassemble::readData() {
  if (id != DISASSEMBLE) {
    stopCPU();

    if (!config.ez80_stopped)
      busy_wait_at_least_cycles(1000);
  }

  cmd_wr_buf[0] = 4 << 16 | PIO_WRITE_ZDI; // Write
  cmd_wr_buf[1] = ZDI_WR_DATA_L << 24 | startAddr.value_u << 16 | startAddr.value_h << 8 | startAddr.value_l; // Read address
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

  if (!config.ez80_stopped && id != DISASSEMBLE)
    startCPU();
}