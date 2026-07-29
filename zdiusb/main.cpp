/******************************************************************************
 *
 * Copyright (c) 2026 by Vitomir Spasojević. All rights reserved.
 *
 ******************************************************************************/

#include <stdio.h>
#include <tusb.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/watchdog.h"
#include "hardware/adc.h"
#include "zdi.pio.h"
#include "circular_buffer.h"
#include "zdi.h"
#include "src/cmd.h"

// +1 byte because tail can go only up to head-1, effectively buffer size is without this byte
#define CIRCULAR_BUFFER_SIZE 4097

uint8_t usb_rx_buf[CFG_TUD_CDC_RX_BUFSIZE];
uint8_t c_buffer[CIRCULAR_BUFFER_SIZE];
cbuf_handle_t me;

PioSm pioSm;

int wr_dma_ch;
int rd_dma_ch;

//void /*__time_critical_func*/ __not_in_flash_func(zdi_task) (PIO pio, uint sm) {
void zdi_task() {
  if (!circular_buf_empty(me)) {
    gpio_put(LED_GPIO, true);
    Cmd* cmd = Cmd::create(me);

    if (cmd != NULL) { // Error message is already printed for cmd == NULL
      ResponseBuf resBuff;

      if (cmd->execute()) {
        resBuff = cmd->getResponse();
      } else {
        resBuff = cmd->getErrResponse();
        printf("Command failed (err: %d)\n", cmd->getErrorCode());
      }

      if (resBuff.startAddr != NULL && tud_cdc_n_connected(0)) {
        uint32_t no = tud_cdc_n_write(0, resBuff.startAddr, resBuff.size);
        tud_cdc_n_write_flush(0 /*itf*/);
      }
    }

    delete cmd;
    gpio_put(LED_GPIO, false);
  }
}

// Callback when data is received on a CDC interface
void tud_cdc_rx_cb(uint8_t itf) {
  //printf("RX CDC %d\n", itf);

  // Read the available data
  uint32_t count = tud_cdc_n_read(itf, usb_rx_buf, sizeof(usb_rx_buf));
  int cbCount = circular_buf_put_range(me, usb_rx_buf, count);
}

int main() {
  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };

  tusb_init(0, &dev_init);
  //tusb_init();
  set_sys_clock_khz(128000, true); // If executed after setup_default_uart(), printf() will be sent as garbage!

  //stdio_init_all();
  setup_default_uart();

  printf("Program version: %s\n", PICO_PROGRAM_VERSION);

  if (watchdog_caused_reboot())
    printf("Rebooted by Watchdog!\n");

  me = circular_buf_init(c_buffer, CIRCULAR_BUFFER_SIZE);

  // Initialize GPIOs
  gpio_init(LED_GPIO);
  gpio_set_dir(LED_GPIO, GPIO_OUT);

  gpio_init(RESET_GPIO);
  gpio_put(RESET_GPIO, 0); // Reset not active
  gpio_set_dir(RESET_GPIO, GPIO_OUT);

  // Initialize ADC
  adc_init();
  adc_gpio_init(VTG_SENSE_GPIO);
  adc_select_input(2); // ADC channel 2 for GPIO 28

  pioSm.pio = pio0;
  pioSm.sm = 0;

  pioSm.offset = pio_add_program(pioSm.pio, &zdi_rw_program); // Returns -1 (PICO_ERROR_GENERIC) if fails
  printf("Loaded program at %u on pio %u\n", pioSm.offset, PIO_NUM(pioSm.pio));

  zdi_rw_program_init(pioSm.pio, pioSm.sm, pioSm.offset, PIO_ZDA_GPIO, ZDI_1MHz);
  pio_sm_set_enabled(pioSm.pio, pioSm.sm, true);

  // DMA read and write channels settings
  wr_dma_ch = dma_claim_unused_channel(true);
  // Write channel configuration
  dma_channel_config c = dma_channel_get_default_config(wr_dma_ch);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, DREQ_PIO0_TX0);

  dma_channel_configure(
    wr_dma_ch,
    &c,
    &pioSm.pio->txf[pioSm.sm], // Write address
    Cmd::cmd_wr_buf, // Read address
    0, // Count will be provided later
    false // Don't start yet
  );

  // Tell the DMA to raise IRQ line 0 when the channel finishes a block
  dma_channel_set_irq0_enabled(wr_dma_ch, true);

  irq_set_exclusive_handler(DMA_IRQ_0, wr_dma_irq_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  rd_dma_ch = dma_claim_unused_channel(true);
  // Read channel configuration
  c = dma_channel_get_default_config(rd_dma_ch);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  channel_config_set_dreq(&c, DREQ_PIO0_RX0);

  dma_channel_configure(
    rd_dma_ch,
    &c,
    Cmd::cmd_rd_buf, // Write address
    &pioSm.pio->rxf[pioSm.sm], // Read address
    0, // Count will be provided later
    false // Don't start yet
  );

  // Tell the DMA to raise IRQ line 0 when the channel finishes a block
  dma_channel_set_irq1_enabled(rd_dma_ch, true);

  irq_set_exclusive_handler(DMA_IRQ_1, rd_dma_irq_handler);
  irq_set_enabled(DMA_IRQ_1, true);
  watchdog_enable(1000, 1);

  while (1) {
    tud_task(); // TinyUSB device task, must be called regularly

    zdi_task();

    watchdog_update();
  }
}