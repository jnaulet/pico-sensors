#ifndef DS18B20_H
#define DS18B20_H

#include "w1.h"
#include "picoRTOS.h"

typedef enum {
    DS18B20_STATE_CMD,
    DS18B20_STATE_DATA,
    DS18B20_STATE_COUNT
} ds18b20_state_t;

struct ds18b20 {
    /*@temp@*/ struct w1 *w1;
    ds18b20_state_t state;
};

int ds18b20_init(/*@out@*/ struct ds18b20 *ctx, struct w1 *w1);
void ds18b20_release(struct ds18b20 *ctx);

/* 1st step */
#define DS18B20_ROM_SEARCH       0xf0
#define DS18B20_ROM_READ         0x33
#define DS18B20_ROM_MATCH        0x55
#define DS18B20_ROM_SKIP         0xcc
#define DS18B20_ROM_ALARM_SEARCH 0xec

/* 2nd step */
#define DS18B20_FN_CONVERT_T     0x44
#define DS18B20_FN_WRITE_SCRATCH 0x4e
#define DS18B20_FN_READ_SCRATCH  0xbe
#define DS18B20_FN_COPY_SCRATCH  0x48
#define DS18B20_FN_RECALL_E2     0xb8
#define DS18B20_FN_READ_PS       0xb4

#define DS18B20_CFG_9BIT  (0 << 5)
#define DS18B20_CFG_10BIT (1 << 5)
#define DS18B20_CFG_11BIT (2 << 5)
#define DS18B20_CFG_12BIT (3 << 5)

int ds18b20_write(struct ds18b20 *ctx, int cmd, /*@null@*/ const void *buf, size_t n);
int ds18b20_read(struct ds18b20 *ctx, int cmd, /*@null@*/ void *buf, size_t n);

#endif
