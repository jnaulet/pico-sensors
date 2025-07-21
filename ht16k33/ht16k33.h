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
};

int ht16k33_init(/*@out@*/ struct ht16k33 *ctx, struct twi *i2c, twi_addr_t addr);
void ht16k33_release(struct ht16k33 *ctx);

#define HT16K33_CC_DISPLAY_DATA  (uint8_t)0x00
#define HT16K33_CC_SYSTEM_SETUP  (uint8_t)0x20
#define HT16K33_CC_KEY_DATA      (uint8_t)0x40
#define HT16K33_CC_INT_FLAG      (uint8_t)0x60
#define HT16K33_CC_DISPLAY_SETUP (uint8_t)0x80
#define HT16K33_CC_ROW_INT_SET   (uint8_t)0xa0
#define HT16K33_CC_DIMMING_SET   (uint8_t)0xef

/* display data address pointer */
#define HT16K33_A_M  0xfu
#define HT16K33_A(x) ((x) & HT16K33_A_M)

/* system setup */
#define HT16K33_S  (1 << 0)

/* display setup */
#define HT16K33_B_M  0x3u
#define HT16K33_B(x) (((x) & HT16K33_B_M) << 1)
#define HT16K33_D    (1 << 0)

#define HT16K33_B_OFF   0
#define HT16K33_B_2HZ   1
#define HT16K33_B_1HZ   2
#define HT16K33_B_0_5HZ 3

/* row/int set */
#define HT16K33_ACT     (1 << 1)
#define HT16K33_ROW_INT (1 << 0)

/* dimming set */
#define HT16K33_P_M  0xfu
#define HT16K33_P(x) ((x) & HT16K33_P_M)

int ht16k33_write(struct ht16k33 *ctx, uint8_t cc, /*@null@*/ const void *buf, size_t n);
int ht16k33_read(struct ht16k33 *ctx, uint8_t cc, void *buf, size_t n);

#endif
