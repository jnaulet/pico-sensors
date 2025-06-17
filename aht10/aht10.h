#ifndef AHT10_H
#define AHT10_H

#include "twi.h"
#include "picoRTOS.h"

#define AHT10_DEFAULT_ADDR (twi_addr_t)0x38

typedef enum {
    AHT10_STATE_SETUP,
    AHT10_STATE_MEASURE_REQ,
    AHT10_STATE_MEASURE_WAIT,
    AHT10_STATE_MEASURE_STATUS,
    AHT10_STATE_CALIBRATION,
    AHT10_STATE_MEASURE_RESP,
    AHT10_STATE_COUNT
} aht10_state_t;

struct aht10 {
    /*@temp@*/ struct twi *i2c;
    twi_addr_t addr;
    /* state machine */
    aht10_state_t state;
    size_t len;
    picoRTOS_tick_t tick;
};

int aht10_init(/*@out@*/ struct aht10 *ctx, struct twi *i2c, twi_addr_t addr);
void aht10_release(struct aht10 *ctx);

struct aht10_measurement {
    int relative_humidity;
    int temperature;
};

int aht10_read(struct aht10 *ctx, /*@out@*/ struct aht10_measurement *m);

#endif
