#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define CFG_TUD_ENABLED         (1)

// Legacy RHPORT configuration
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT        (0)
#endif
// end legacy RHPORT

//------------------------
// DEVICE CONFIGURATION //
//------------------------

// Enable 1 CDC class
#define CFG_TUD_CDC             (1)
// Set CDC FIFO buffer sizes
#define CFG_TUD_CDC_RX_BUFSIZE  (512)
#define CFG_TUD_CDC_TX_BUFSIZE  (512)
#define CFG_TUD_CDC_EP_BUFSIZE  (512)

//rx_unread_buf_sz = 256

// #ifndef CFG_TUD_ENDPOINT0_SIZE
// #define CFG_TUD_ENDPOINT0_SIZE  (512)
// #endif

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */