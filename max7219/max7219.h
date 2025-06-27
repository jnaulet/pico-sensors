#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>
#include <spi.h>

typedef enum {
    MAX7219_ADDR_NOOP           = 0x0,
    MAX7219_ADDR_DIGIT0         = 0x1,
    MAX7219_ADDR_DIGIT1         = 0x2,
    MAX7219_ADDR_DIGIT2         = 0x3,
    MAX7219_ADDR_DIGIT3         = 0x4,
    MAX7219_ADDR_DIGIT4         = 0x5,
    MAX7219_ADDR_DIGIT5         = 0x6,
    MAX7219_ADDR_DIGIT6         = 0x7,
    MAX7219_ADDR_DIGIT7         = 0x8,
    MAX7219_ADDR_DECODE_MODE    = 0x9,
    MAX7219_ADDR_INTENSITY      = 0xa,
    MAX7219_ADDR_SCAN_LIMIT     = 0xb,
    MAX7219_ADDR_SHUTDOWN       = 0xc,
    MAX7219_ADDR_DISPLAY_TEST   = 0xf,
    MAX7219_ADDR_COUNT
} max7219_addr_t;

typedef enum {
    MAX7219_STATE_SETUP,
    MAX7219_STATE_XFER,
    MAX7219_STATE_COUNT
} max7219_state_t;

struct max7219 {
    /*@temp@*/ struct spi *spi;
    /*@temp@*/ struct gpio *load;
    max7219_state_t state;
    size_t len;
};

int max7219_init(/*@out@*/ struct max7219 *ctx, struct spi *spi, struct gpio *load);
void max7219_release(struct max7219 *ctx);

int max7219_send(struct max7219 *ctx, max7219_addr_t addr, uint8_t data);

#endif
