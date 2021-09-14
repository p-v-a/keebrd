#ifndef LED_H
#define LED_H
#include "stdint.h"

/* keyboard LEDs */
#define USB_LED_NUM_LOCK                0
#define USB_LED_CAPS_LOCK               1
#define USB_LED_SCROLL_LOCK             2

#ifdef __cplusplus
extern "C" {
#endif

void led_set(uint8_t usb_led);

#ifdef __cplusplus
}
#endif

#endif
