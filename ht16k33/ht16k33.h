#ifndef HT16K33_H
#define HT16K33_H

#include "twi.h"
#include "picoRTOS.h"

#define HT16K33_BASEADDR       (twi_addr_t)0x70
#define HT16K33_BASEADDR_COUNT 8

typedef enum {
    HT16K33_STATE_SETUP,
    HT16K33_STATE_CC,
    HT16K33_STATE_DATA,
    HT16K33_STATE_COUNT
} ht16k33_state_t;

struct ht16k33 {
    /*@temp@*/ struct twi *i2c;
    twi_addr_t addr;
    /* state machine */
    ht16k33_state_t state;
    size_t len;
    picoRTOS_tick_t tick;
};

int ht16k33_init(/*@out@*/ struct ht16k33 *ctx, struct twi *i2c, twi_addr_t addr);
void ht16k33_release(struct ht16k33 *ctx);

#define HT16K33_CC_DISPLAY_DATA  0x00
#define HT16K33_CC_SYSTEM_SETUP  0x20
#define HT16K33_CC_KEY_DATA      0x40
#define HT16K33_CC_INT_FLAG      0x60
#define HT16K33_CC_DISPLAY_SETUP 0x80
#define HT16K33_CC_ROW_INT_SET   0xa0
#define HT16K33_CC_DIMMING_SET   0xef

#define DISPLAY_DATA_M  0xfu
#define DISPLAY_DATA(x) ((x) & DISPLAY_DATA_M)

#define SYSTEM_SETUP_S  (1 << 0)

#define DISPLAY_SETUP_D    (1 << 2)
#define DISPLAY_SETUP_B_M  0x3u
#define DISPLAY_SETUP_B(x) ((x) & DISPLAY_SETUP_B_M)

#define B_OFF   0
#define B_2HZ   1
#define B_1HZ   2
#define B_0_5HZ 3

#define ACT     (1 << 1)
#define ROW_INT (1 << 0)

#define P_M  0xfu
#define P(x) ((x) & P_M)

int ht16k33_write(struct ht16k33 *ctx, uint8_t cc, /*@null@*/ const void *buf, size_t n);
int ht16k33_read(struct ht16k33 *ctx, uint8_t cc, void *buf, size_t n);

#endif
