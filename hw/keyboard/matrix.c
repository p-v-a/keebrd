#include <stdint.h>

#include "matrix.h"
#include "../system.h"
#include "../pin_manager.h"

//#include <libpic30.h>

static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

bool matrix_has_ghost_in_row(uint8_t row)
{
    uint8_t i;
    matrix_row_t matrix_row = matrix_get_row(row);
    // No ghost exists when less than 2 keys are down on the row
    if (((matrix_row - 1) & matrix_row) == 0)
        return false;

    // Ghost occurs when the row shares column line with other row
    for (i=0; i < MATRIX_ROWS; i++) {
        if (i != row && (matrix_get_row(i) & matrix_row))
            return true;
    }
    return false;
}

uint8_t matrix_rows(void)
{
    return MATRIX_ROWS;
}

uint8_t matrix_cols(void)
{
    return MATRIX_COLS;
}

void matrix_clear(void)
{
}

void matrix_setup(void)
{
    uint8_t i;
    // initialize row and col
    unselect_rows();

    // initialize matrix state: all keys off
    for (i=0; i < MATRIX_ROWS; i++) {
        matrix[i] = 0;
        matrix_debouncing[i] = 0;
    }
}

uint8_t matrix_scan(void)
{
    uint8_t i;
    for (i = 0; i < MATRIX_ROWS; i++) {
        select_row(i);
//        __delay_us(30);
//        wait_us(30);  // without this wait read unstable value.
        matrix_row_t cols = read_cols();
        if (matrix_debouncing[i] != cols) {
            matrix_debouncing[i] = cols;
            debouncing = DEBOUNCE;
        }
        unselect_rows();
    }

    if (debouncing) {
        if (--debouncing) {
//            wait_ms(1);
        } else {
            uint8_t i;
            for (i = 0; i < MATRIX_ROWS; i++) {
                matrix[i] = matrix_debouncing[i];
            }
        }
    }

    return 1;
}

/* Returns status of switches(1:on, 0:off) */
static matrix_row_t read_cols(void)
{
    return (
          ((matrix_row_t) COL0_GetValue() << 0) |
          ((matrix_row_t) COL1_GetValue() << 1) |
          ((matrix_row_t) COL2_GetValue() << 2) |
          ((matrix_row_t) COL3_GetValue() << 3) |
          ((matrix_row_t) COL4_GetValue() << 4) |
          ((matrix_row_t) COL5_GetValue() << 5) |
          ((matrix_row_t) COL6_GetValue() << 6) |
          ((matrix_row_t) COL7_GetValue() << 7) |
          ((matrix_row_t) COL8_GetValue() << 8) |
          ((matrix_row_t) COL9_GetValue() << 9) |
          ((matrix_row_t) COL10_GetValue() << 10) |
          ((matrix_row_t) COL11_GetValue() << 11) |
          ((matrix_row_t) COL12_GetValue() << 12) |
          ((matrix_row_t) COL13_GetValue() << 13) |
          ((matrix_row_t) COL14_GetValue() << 14) |
          ((matrix_row_t) COL15_GetValue() << 15) |
          ((matrix_row_t) COL16_GetValue() << 16) |
          ((matrix_row_t) COL17_GetValue() << 17)
    );
}

/* Row pin configuration
 */
static void unselect_rows(void)
{
    ROW0_SetLow();
    ROW1_SetLow();
    ROW2_SetLow();
    ROW3_SetLow();
    ROW4_SetLow();
    ROW5_SetLow();
    ROW6_SetLow();
    ROW7_SetLow();
}

static void select_row(uint8_t row)
{
    (void)row;
    // Output low to select
    switch (row) {
        case 0:
            ROW0_SetHigh();
            break;
        case 1:
            ROW1_SetHigh();
            break;
        case 2:
            ROW2_SetHigh();
            break;
        case 3:
            ROW3_SetHigh();
            break;
        case 4:
            ROW4_SetHigh();
            break;
        case 5:
            ROW5_SetHigh();
            break;
        case 6:
            ROW6_SetHigh();
            break;
        case 7:
            ROW7_SetHigh();
            break;
    }
}

matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}
