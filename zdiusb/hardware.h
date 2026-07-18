#ifndef HARDWARE_H
#define HARDWARE_H

typedef enum : uint8_t {
  // ZDI write-only registers
  ZDI_ADDR0L      = 0x00,
  ZDI_ADDR0H      = 0x01 << 1,
  ZDI_ADDR0U      = 0x02 << 1,
  ZDI_ADDR1L      = 0x04 << 1,
  ZDI_ADDR1H      = 0x05 << 1,
  ZDI_ADDR1U      = 0x06 << 1,
  ZDI_ADDR2L      = 0x08 << 1,
  ZDI_ADDR2H      = 0x09 << 1,
  ZDI_ADDR2U      = 0x0A << 1,
  ZDI_ADDR3L      = 0x0C << 1,
  ZDI_ADDR3H      = 0x0D << 1,
  ZDI_ADDR3U      = 0x0E << 1,
  ZDI_BRK_CTL     = 0x10 << 1,
  ZDI_MASTER_CTL  = 0x11 << 1,
  ZDI_WR_DATA_L   = 0x13 << 1,
  ZDI_WR_DATA_H	  = 0x14 << 1,
  ZDI_WR_DATA_U	  = 0x15 << 1,
  ZDI_RW_CTL      = 0x16 << 1,
  ZDI_BUS_CTL     = 0x17 << 1,
  ZDI_IS4	        = 0x21 << 1,
  ZDI_IS3	        = 0x22 << 1,
  ZDI_IS2	        = 0x23 << 1,
  ZDI_IS1	        = 0x24 << 1,
  ZDI_IS0	        = 0x25 << 1,
  ZDI_WR_MEM      = 0x30 << 1,
  // ZDI read-only registers
  ZDI_ID_L        = (0x00 << 1) | 0x01,
  ZDI_ID_H        = (0x01 << 1) | 0x01,
  ZDI_ID_REV      = (0x02 << 1) | 0x01,
  ZDI_STAT        = (0x03 << 1) | 0x01,
  ZDI_RD_DATA_L	  = (0x10 << 1) | 0x01,
  ZDI_RD_DATA_H	  = (0x11 << 1) | 0x01,
  ZDI_RD_DATA_U	  = (0x12 << 1) | 0x01,
  ZDI_BUS_STAT    = (0x17 << 1) | 0x01,
  ZDI_RD_MEM      = (0x20 << 1) | 0x01,
  // Probe commands
  HW_RESET        = 0x80
} CmdCode;

// ZDI_BRK_CTL bit values
#define	ZDI_BRK_NEXT    0x80
#define	ZDI_BRK_ADDR3	  0x40 // 0 - Disabled, 1 - Enabled
#define	ZDI_BRK_ADDR2	  0x20
#define	ZDI_BRK_ADDR1	  0x10
#define	ZDI_BRK_ADDR0	  0x08
#define	ZDI_IGN_LOW_1	  0x04
#define	ZDI_IGN_LOW_0	  0x02
#define	ZDI_SINGLE_STEP	0x01

// ZDI_MASTER_CTL bit values
#define	ZDI_RESET	      0x80

// ZDI_RW_CTL bit values
#define	ZDI_REG_AF      0x00
#define	ZDI_REG_BC      0x01
#define	ZDI_REG_DE      0x02
#define	ZDI_REG_HL      0x03
#define	ZDI_REG_IX      0x04
#define	ZDI_REG_IY      0x05
#define	ZDI_REG_SP      0x06
#define	ZDI_REG_PC      0x07
#define ADL_SET         0x08 // ADL = 1
#define ADL_CLR         0x09 // ADL = 0
#define REG_EX          0x0A // Exchange register set
#define	ZDI_REG_MEM	    0x0B // Read memory from current PC address
#define	ZDI_REG_WR      0x80 // In addition to register name (ZDI_REG_AF,...) for write operation. If MSB is zero, it is the read operation.
#define REG_RD_AF       ZDI_REG_AF
#define REG_RD_BC       ZDI_REG_BC
#define REG_RD_DE       ZDI_REG_DE
#define REG_RD_HL       ZDI_REG_HL
#define REG_RD_IX       ZDI_REG_IX
#define REG_RD_IY       ZDI_REG_IY
#define REG_RD_SP       ZDI_REG_SP
#define REG_RD_PC       ZDI_REG_PC
#define MEM_RD_PC       ZDI_REG_MEM
#define REG_WR_AF       (ZDI_REG_WR | ZDI_REG_AF)
#define REG_WR_BC       (ZDI_REG_WR | ZDI_REG_BC)
#define REG_WR_DE       (ZDI_REG_WR | ZDI_REG_DE)
#define REG_WR_HL       (ZDI_REG_WR | ZDI_REG_HL)
#define REG_WR_IX       (ZDI_REG_WR | ZDI_REG_IX)
#define REG_WR_IY       (ZDI_REG_WR | ZDI_REG_IY)
#define REG_WR_SP       (ZDI_REG_WR | ZDI_REG_SP)
#define REG_WR_PC       (ZDI_REG_WR | ZDI_REG_PC)
#define MEM_WR_PC       (ZDI_REG_WR | ZDI_REG_MEM)

// ZDI_BUS_CTL bit values
#define	ZDI_BUSACK_EN_WR 0x80
#define	ZDI_BUSACK	    0x40

// ZDI_STAT bit values
#define	ZDI_ACTIVE      0x80
#define	ZDI_HALT_SLP    0x20
#define	ZDI_ADL	        0x10
#define	ZDI_MADL        0x08
#define	ZDI_IEF1        0x04

// ZDI_BUS_STAT bit values
#define	ZDI_BUSACK_EN	  0x80
#define	ZDI_BUSACK_STAT 0x40

// Register save mask
#define	SAVE_AF	  (1 << ZDI_REG_AF)
#define	SAVE_BC	  (1 << ZDI_REG_BC)
#define	SAVE_DE	  (1 << ZDI_REG_DE)
#define	SAVE_HL	  (1 << ZDI_REG_HL)
#define	SAVE_IX	  (1 << ZDI_REG_IX)
#define	SAVE_IY	  (1 << ZDI_REG_IY)
#define	SAVE_SP	  (1 << ZDI_REG_SP)
#define	SAVE_PC	  (1 << ZDI_REG_PC)
#define	SAVE_ALL_REGS	(SAVE_AF | SAVE_BC | SAVE_DE | SAVE_HL | SAVE_IX | SAVE_IY | SAVE_SP | SAVE_PC)

/////////////////////////////////////
// GPIO settings
/////////////////////////////////////

#define RESET_GPIO 8
// ZDA GPIO = 10, ZCL_GPIO = 11, ZDA_DIR = 12, ZCL_DIR = 13 (high - out, low - in)
// Set pins: 1
// Out pins: 1
// Side_set pins: 3
// Pindirs consecutive pins: 4
#define PIO_ZDA_GPIO 10
//#define PIO_ZCL_GPIO 11
//#define ZDA_DIR_GPIO 12 // Direction control for ZDA transceiver pin
//#define ZCL_DIR_GPIO 13
#define LED_GPIO 25
#define VTG_SENSE_GPIO 28

#endif // HARDWARE_H
