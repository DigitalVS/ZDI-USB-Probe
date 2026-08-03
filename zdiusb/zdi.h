#ifndef ZDI_H
#define ZDI_H

#include <stdint.h>

// Speed value is a clock divider (each ZCL cycle state, high or low, takes 2 clock cycles, so one period is 4 ticks)
typedef enum {ZDI_1MHz = 32, ZDI_2MHz = 16, ZDI_4MHz = 8, ZDI_8MHz = 4} ZdiSpeed;

typedef struct {
  PIO pio;
  uint sm;
  uint offset;
} PioSm;

// Command codes for PIO for read or write to ZDI
// Value is PIO program address for read or write operation
#define PIO_READ_ZDI 11 + pioSm.offset
#define PIO_WRITE_ZDI  5 + pioSm.offset

struct Uint24 {
  Uint24() : value_l(0), value_h(0), value_u(0) {
  }

  Uint24(uint8_t value_l, uint8_t value_h, uint8_t value_u) {
    this->value_l = value_l;
    this->value_h = value_h;
    this->value_u = value_u;
  }

  uint8_t value_l;
  uint8_t value_h;
  uint8_t value_u;
};

typedef struct {
  Uint24 pc_reg; // 24 bit PC register backup value. Byte order is: L, H, U.
  uint8_t zdi_status; // ZDI status register (ZDI_STAT) value
  uint8_t ez80_stopped; // Value is one if ZDI_ACTIVE bit in ZDI_STAT register is set
  uint8_t adl_mode;
  ZdiSpeed zdi_speed;
  // Breakpoint addresses
  Uint24 break_addresses[4];
  uint8_t break_enabled[4];
} Config;

void wr_dma_irq_handler();
void rd_dma_irq_handler();

#endif // ZDI_H