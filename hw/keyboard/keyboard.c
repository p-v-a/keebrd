/**
  @File Name
    main.c

  @Summary
    This source file provides keyboard handling routines.

  @Description
    This source file provides keyboard handling routines.
*/
#include <stdint.h>

#include "../system.h"
#include "../pin_manager.h"
#include "keyboard.h"
#include "keycode.h"
#include "matrix.h"

static const uint8_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = {
        /*
         0        1        2         3        4          5          6        7        8            9               10           11      12      13    14    15     16       17*/
/* 0 */ {KC_NO,   KC_NO,   KC_F5,   KC_PSCR, KC_0,     KC_F10,    KC_NO,   KC_NO,   KC_NO,       KC_PGDN,        KC_END,      KC_9,   KC_8,    KC_7, KC_4, KC_3,  KC_2,    KC_1 },
/* 1 */ {KC_NO,   KC_NO,   KC_LCTL, KC_NO,   KC_MINUS, KC_F9,     KC_NO,   KC_DEL,  KC_INS,      KC_PGUP,        KC_HOME,     KC_F8,  KC_EQL,  KC_6, KC_5, KC_F2, KC_F1,   KC_GRV},
/* 2 */ {KC_NO,   KC_NO,   KC_NO,   KC_RALT, KC_SLSH,  KC_F12,    KC_NO,   KC_DOWN, KC_RIGHT,    KC_KP_MINUS,    KC_LEFT,     KC_APP, KC_NO,   KC_N, KC_B, KC_NO, KC_NO,   KC_NO },
/* 3 */ {KC_NO,   KC_NO,   KC_RCTL, KC_NO,   KC_NO,    KC_ENT,    KC_NO,   KC_COMM, KC_KP_SLASH, KC_KP_ASTERISK, KC_NO,       KC_DOT, KC_NLCK, KC_M, KC_V, KC_C,  KC_X,    KC_Z},
/* 4 */ {KC_NO,   KC_NO,   KC_NO,   KC_LALT, KC_QUOTE, KC_F11,    KC_NO,   KC_SPC,  KC_KP_0,     KC_KP_DOT,      KC_UP,       KC_NO,  KC_F6,   KC_H, KC_G, KC_F4, KC_NO,   KC_ESC},
/* 5 */ {KC_RGUI, KC_NO,   KC_NO,   KC_NO,   KC_SCLN,  KC_BSLASH, KC_RSFT, KC_KP_1, KC_KP_2,     KC_KP_3,        KC_KP_ENTER, KC_L,   KC_K,    KC_J, KC_F, KC_D,  KC_S,    KC_A},
/* 6 */ {KC_NO,   KC_LGUI, KC_NO,   KC_NO,   KC_LBRC,  KC_BSPC,   KC_LSFT, KC_KP_4, KC_KP_5,     KC_KP_6,        KC_NO,       KC_F7,  KC_RBRC, KC_Y, KC_T, KC_F3, KC_CAPS, KC_TAB },
/* 7 */ {KC_NO,   KC_NO,   KC_PAUS, KC_SLCK, KC_P,     KC_NO,     KC_NO,   KC_KP_7, KC_KP_8,     KC_KP_9,        KC_KP_PLUS,  KC_O,   KC_I,    KC_U, KC_R, KC_E,  KC_W,    KC_Q}
    }
};

void keyboard_set_leds(uint8_t usb_led)
{
    (usb_led & (1 << USB_LED_CAPS_LOCK)) != 0 ? LED_CAPS_SetHigh() : LED_CAPS_SetLow();
    (usb_led & (1 << USB_LED_NUM_LOCK)) != 0 ? LED_NUM_SetHigh() : LED_NUM_SetLow();
    (usb_led & (1 << USB_LED_SCROLL_LOCK)) != 0 ? LED_SCRLL_SetHigh() : LED_SCRLL_SetLow();
}

/*
 * Do keyboard routine jobs: scan matrix, light LEDs, ...
 * This is repeatedly called as fast as possible.
 */
void keyboard_task(action_t action)
{
    static matrix_row_t matrix_prev[MATRIX_ROWS];
    static matrix_row_t matrix_ghost[MATRIX_ROWS];

    //static uint8_t led_status = 0;
    matrix_row_t matrix_row = 0;
    matrix_row_t matrix_change = 0;

    matrix_scan();
    
    uint8_t r;
    for (r = 0; r < MATRIX_ROWS; r++) {
        matrix_row = matrix_get_row(r);
        matrix_change = matrix_row ^ matrix_prev[r];
        if (matrix_change) {
            if (matrix_has_ghost_in_row(r)) {
                /* Keep track of whether ghosted status has changed for
                 * debugging. But don't update matrix_prev until un-ghosted, or
                 * the last key would be lost.
                 */
                matrix_ghost[r] = matrix_row;
                continue;
            }
            matrix_ghost[r] = matrix_row;
            matrix_row_t col_mask = 1;

            uint8_t c;
            for (c = 0; c < MATRIX_COLS; c++, col_mask <<= 1) {
                if (matrix_change & col_mask) {
                    keyevent_t e = (keyevent_t){
                        .key = (keypos_t){ .row = r, .col = c },
                        .keycode = keymaps[0][r][c],
                        .pressed = (matrix_row & col_mask)
                    };

                    action(e);

                    // record a processed key
                    matrix_prev[r] ^= col_mask;
                }
            }
        }
    }

    // update LED
    /*if (led_status != host_keyboard_leds()) {
        led_status = host_keyboard_leds();
        if (debug_keyboard) dprintf("LED: %02X\n", led_status);
        hook_keyboard_leds_change(led_status);
    }*/
}