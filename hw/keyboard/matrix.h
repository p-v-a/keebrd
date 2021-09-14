#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

/* key matrix size */
#define MATRIX_ROWS 8
#define MATRIX_COLS 18

#if (MATRIX_COLS <= 8)
typedef  uint8_t    matrix_row_t;
#elif (MATRIX_COLS <= 16)
typedef  uint16_t   matrix_row_t;
#elif (MATRIX_COLS <= 32)
typedef  uint32_t   matrix_row_t;
#else
#error "MATRIX_COLS: invalid value"
#endif

#if (MATRIX_ROWS > 255)
#error "MATRIX_ROWS must not exceed 255"
#endif

/* Set 0 if debouncing isn't needed */
#define DEBOUNCE    5

/* number of matrix rows */
uint8_t matrix_rows(void);

/* number of matrix columns */
uint8_t matrix_cols(void);

/* should be called at early stage of startup before matrix_init.(optional) */
void matrix_setup(void);

/* intialize matrix for scaning. */
void matrix_init(void);

/* scan all key states on matrix */
uint8_t matrix_scan(void);

/* whether modified from previous scan. used after matrix_scan. */
bool matrix_is_modified(void) __attribute__ ((deprecated));

/* whether a swtich is on */
bool matrix_is_on(uint8_t row, uint8_t col);

/* matrix state on row */
matrix_row_t matrix_get_row(uint8_t row);

/* print matrix for debug */
void matrix_print(void);

/* clear matrix */
void matrix_clear(void);

bool matrix_has_ghost_in_row(uint8_t row);

static matrix_row_t read_cols(void);
static void unselect_rows(void);
static void select_row(uint8_t row);

#endif