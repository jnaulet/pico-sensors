#ifndef BME280_H
#define BME280_H

#include "twi.h"
#include "picoRTOS.h"

#include <stdint.h>

#define BME280_DEFAULT_ADDR (twi_addr_t)0x76

typedef enum {
    BME280_STATE_SETUP,
    BME280_STATE_SOFTRESET,
    BME280_STATE_TRIMMING_00,
    BME280_STATE_TRIMMING_26,
    BME280_STATE_SLEEP,
    BME280_STATE_FORCE,
    BME280_STATE_READ,
    BME280_STATE_COUNT
} bme280_state_t;

struct bme280 {
    /*@temp@*/ struct twi *i2c;
    twi_addr_t addr;
    /* state machine */
    bme280_state_t state;
    uint8_t reg;
    struct {
        int n;
        size_t total;
    } xfer;
    /* compensation values */
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
    uint8_t dig_h1;
    int16_t dig_h2;
    uint8_t dig_h3;
    int16_t dig_h4;
    int16_t dig_h5;
    int8_t dig_h6;
    /* humidity */
    int32_t t_fine;
};

int bme280_init(/*@out@*/ struct bme280 *ctx, struct twi *i2c, twi_addr_t addr);
void bme280_release(struct bme280 *ctx);

struct bme280_measurement {
    uint32_t relative_humidity; /* / 1024 */
    int32_t temperature;        /* / 100 */
    int32_t pressure;
};

int bme280_read(struct bme280 *ctx, /*@out@*/ struct bme280_measurement *m);

#endif
