#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

/* keyboard LEDs */
#define USB_LED_NUM_LOCK                0
#define USB_LED_CAPS_LOCK               1
#define USB_LED_SCROLL_LOCK             2

/* key matrix position */
typedef struct {
    uint8_t col;
    uint8_t row;
} keypos_t;

/* key event */
typedef struct {
    keypos_t key;
    uint8_t  keycode;
    bool     pressed;
} keyevent_t;

typedef void (*action_t)(keyevent_t);

/* it runs once at early stage of startup before keyboard_init. */
void keyboard_setup(void);
/* it runs once after initializing host side protocol, debug and MCU peripherals. */
void keyboard_init(void);
/* it runs repeatedly in main loop */
void keyboard_task(action_t);
/* it runs when host LED status is updated */
void keyboard_set_leds(uint8_t leds);

#endif

