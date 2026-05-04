#include "bme280.h"
#include <errno.h>

#define BME280_REG_CALIB00   0x88
#define BME280_REG_ID        0xd0
#define BME280_REG_RESET     0xe0
#define BME280_REG_CALIB26   0xe1
#define BME280_REG_CTRL_HUM  0xf2
#define BME280_REG_STATUS    0xf3
#define BME280_REG_CTRL_MEAS 0xf4
#define BME280_REG_CONFIG    0xf5
#define BME280_REG_PRESS     0xf7
#define BME280_REG_TEMP      0xfa
#define BME280_REG_HUM       0xfd

int bme280_init(struct bme280 *ctx, struct twi *i2c, twi_addr_t addr)
{
    ctx->i2c = i2c;
    ctx->addr = addr;

    ctx->state = BME280_STATE_SETUP;
    ctx->reg = (uint8_t)BME280_REG_CALIB00;

    ctx->xfer.n = 0;
    ctx->xfer.total = 0;

    return 0;
}

void bme280_release(struct bme280 *ctx)
{
    /*@i@*/ (void)ctx;
}

static int twi_write_reg(struct bme280 *ctx, uint8_t reg, uint8_t value)
{
    if (ctx->xfer.total == 0) {
        ctx->xfer.total = (size_t)1;
        ctx->xfer.n = -1;
    }

    if (ctx->xfer.n < 0) {
        int res;
        if ((res = twi_write(ctx->i2c, &reg, sizeof(reg) + 1,
                             TWI_F_S | TWI_F_P | TWI_F_N(1))) < 0)
            return res;

        /* confirm */
        ctx->xfer.n = 0;
    }

    if (ctx->xfer.n != -1) {
        int res;
        if ((res = twi_write(ctx->i2c, &value, (size_t)1,
                             TWI_F_S | TWI_F_P)) < 0)
            return res;

        ctx->xfer.total = 0;
        return 1;
    }

    return -EAGAIN;
}

static int twi_read_reg(struct bme280 *ctx, uint8_t reg, uint8_t *buf, size_t n)
{
    if (ctx->xfer.total == 0) {
        ctx->xfer.total = n;
        ctx->xfer.n = -1;
    }

    if (ctx->xfer.n < 0) {
        int res;
        if ((res = twi_write(ctx->i2c, &reg, sizeof(reg), TWI_F_S)) < 0)
            return res;

        /* confirm */
        ctx->xfer.n = 0;
    }

    if (ctx->xfer.n != -1) {
        int res;
        if ((res = twi_read(ctx->i2c, &buf[ctx->xfer.n],
                            ctx->xfer.total - (size_t)ctx->xfer.n,
                            TWI_F_S | TWI_F_P)) < 0)
            return res;

        ctx->xfer.n += res;
        if (ctx->xfer.n == (int)ctx->xfer.total) {
            ctx->xfer.total = 0;
            return (int)ctx->xfer.n;
        }
    }

    return -EAGAIN;
}

static void compensation_t(struct bme280 *ctx, int32_t t, struct bme280_measurement *m)
{
    int32_t x;
    int32_t y;

    x = (t / 8) - ((int32_t)ctx->dig_t1 * 2);
    x = (x * ((int32_t)ctx->dig_t2)) / 2048;
    y = (t / 16) - (int32_t)ctx->dig_t1;
    y = (((y * y) / 4096) * (int32_t)ctx->dig_t3) / 16384;

    ctx->t_fine = (x + y);
    m->temperature = (ctx->t_fine * 5 + 128) / 256;
}

static void compensation_h(struct bme280 *ctx, int32_t h, struct bme280_measurement *m)
{
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
    int32_t e;

    a = ctx->t_fine - (int32_t)76800l;
    b = h * 16384;
    c = (int32_t)ctx->dig_h4 * (int32_t)1048576l;
    d = (int32_t)ctx->dig_h5 * a;
    e = (((b - c) - d) + 16384) / 32768;
    b = (a * (int32_t)ctx->dig_h6) / 1024;
    c = (a * (int32_t)ctx->dig_h3) / 2048;
    d = ((b * (c + (int32_t)32768)) / 1024) + (int32_t)2097152l;
    b = ((d * (int32_t)ctx->dig_h2) + 8192) / 16384;
    c = e * b;
    d = ((c / 32768) * (c / 32768)) / 128;
    e = c - ((d * (int32_t)ctx->dig_h1) / 16);
    e = (e < 0 ? 0 : e);
    e = (e > (int32_t)419430400l ? (int32_t)419430400l : e);

    m->relative_humidity = (uint32_t)(e / 4096);
}

static int bme280_read_measurement(struct bme280 *ctx, struct bme280_measurement *m)
{
#define HUM_TEMP_PRESS_COUNT 8
    int res;
    static uint8_t meas[HUM_TEMP_PRESS_COUNT];

    if ((res = twi_read_reg(ctx, (uint8_t)BME280_REG_PRESS, meas, sizeof(meas))) < 0)
        return res;

    /* decode measurements */
    // uint32_t p = (uint32_t)meas[0] << 12 | (uint32_t)meas[1] << 4 | meas[2];
    uint32_t t = (uint32_t)meas[3] << 12 | (uint32_t)meas[4] << 4 | meas[5];
    uint16_t h = (uint16_t)meas[6] << 8 | meas[7];

    /* apply calibrations */
    compensation_t(ctx, (int32_t)t, m);
    // TODO: compensation_p(ctx, (int32_t)p, m);
    compensation_h(ctx, (int32_t)h, m);

    ctx->state = BME280_STATE_SLEEP;
    return (int)sizeof(*m);
}

static int bme280_force_measurement(struct bme280 *ctx, struct bme280_measurement *m)
{
    /* max oversampling */
    if (ctx->reg == (uint8_t)BME280_REG_CTRL_HUM) {
        int res;
        if ((res = twi_write_reg(ctx, ctx->reg, (uint8_t)0x3)) < 0)
            return res;

        ctx->reg = (uint8_t)BME280_REG_CTRL_MEAS;
    }

    if (ctx->reg == (uint8_t)BME280_REG_CTRL_MEAS) {
        int res;
        if ((res = twi_write_reg(ctx, ctx->reg, (uint8_t)0xfe)) < 0)
            return res;

        ctx->reg = (uint8_t)BME280_REG_STATUS;
    }

    if (ctx->reg == (uint8_t)BME280_REG_STATUS) {
        int res;
        uint8_t status = 0;
        if ((res = twi_read_reg(ctx, ctx->reg, &status, sizeof(status))) < 0)
            return res;

        if (status == 0) {
            ctx->state = BME280_STATE_READ;
            return bme280_read_measurement(ctx, m);
        }
    }

    return -EAGAIN;
}

static int bme280_read_trimming_26(struct bme280 *ctx, struct bme280_measurement *m)
{
#define CALIB26_41_COUNT 16
    int res;
    static uint8_t calib[CALIB26_41_COUNT];

    if ((res = twi_read_reg(ctx, (uint8_t)BME280_REG_CALIB26, calib, sizeof(calib))) < 0)
        return res;

    /* apply calibrations */
    ctx->dig_h2 = (int16_t)((uint16_t)calib[1] << 8 | calib[0]);
    ctx->dig_h3 = (uint8_t)calib[2];
    ctx->dig_h4 = (int16_t)((uint16_t)calib[3] << 4 | (0xf & calib[4]));
    ctx->dig_h5 = (int16_t)((uint16_t)calib[5] << 4 | (calib[4] >> 4));
    ctx->dig_h6 = (int8_t)calib[6];

    ctx->state = BME280_STATE_FORCE;
    ctx->reg = (uint8_t)BME280_REG_CTRL_HUM;
    return bme280_force_measurement(ctx, m);
}

static int bme280_read_trimming_00(struct bme280 *ctx, struct bme280_measurement *m)
{
#define CALIB00_25_COUNT 26
    int res;
    static uint8_t calib[CALIB00_25_COUNT];

    if ((res = twi_read_reg(ctx, (uint8_t)BME280_REG_CALIB00, calib, sizeof(calib))) < 0)
        return res;

    /* apply calibrations */
    ctx->dig_t1 = (uint16_t)calib[1] << 8 | calib[0];
    ctx->dig_t2 = (int16_t)((uint16_t)calib[3] << 8 | calib[2]);
    ctx->dig_t3 = (int16_t)((uint16_t)calib[5] << 8 | calib[4]);
    ctx->dig_p1 = (uint16_t)calib[7] << 8 | calib[6];
    ctx->dig_p2 = (int16_t)((uint16_t)calib[9] << 8 | calib[8]);
    ctx->dig_p3 = (int16_t)((uint16_t)calib[11] << 8 | calib[10]);
    ctx->dig_p4 = (int16_t)((uint16_t)calib[13] << 8 | calib[12]);
    ctx->dig_p5 = (int16_t)((uint16_t)calib[15] << 8 | calib[14]);
    ctx->dig_p6 = (int16_t)((uint16_t)calib[17] << 8 | calib[16]);
    ctx->dig_p7 = (int16_t)((uint16_t)calib[19] << 8 | calib[18]);
    ctx->dig_p8 = (int16_t)((uint16_t)calib[21] << 8 | calib[20]);
    ctx->dig_p9 = (int16_t)((uint16_t)calib[23] << 8 | calib[22]);
    ctx->dig_h1 = (uint8_t)calib[25];

    ctx->state = BME280_STATE_TRIMMING_26;
    return bme280_read_trimming_26(ctx, m);
}

static int bme280_read_softreset(struct bme280 *ctx, struct bme280_measurement *m)
{
    if (ctx->reg == (uint8_t)BME280_REG_RESET) {
        int res;
        if ((res = twi_write_reg(ctx, (uint8_t)BME280_REG_RESET, (uint8_t)0xb6)) < 0)
            return res;

        ctx->reg = (uint8_t)BME280_REG_STATUS;
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(2));
    }

    if (ctx->reg == (uint8_t)BME280_REG_STATUS) {
        int res;
        uint8_t status = 0;
        if ((res = twi_read_reg(ctx, ctx->reg, &status, sizeof(status))) < 0)
            return res;

        if (status == 0) {
            ctx->state = BME280_STATE_TRIMMING_00;
            return bme280_read_trimming_00(ctx, m);
        }
    }

    return -EAGAIN;
}

static int bme280_read_setup(struct bme280 *ctx, struct bme280_measurement *m)
{
    int res;
    struct twi_settings i2c_settings = {
        TWI_BITRATE_STANDARD,
        TWI_MODE_MASTER,
        ctx->addr
    };

    if ((res = twi_setup(ctx->i2c, &i2c_settings)) < 0)
        return res;

    ctx->state = BME280_STATE_SOFTRESET;
    ctx->reg = (uint8_t)BME280_REG_RESET;
    return bme280_read_softreset(ctx, m);
}

static int bme280_read_sleep(struct bme280 *ctx, struct bme280_measurement *m)
{
    int res;
    struct twi_settings i2c_settings = {
        TWI_BITRATE_STANDARD,
        TWI_MODE_MASTER,
        ctx->addr
    };

    if ((res = twi_setup(ctx->i2c, &i2c_settings)) < 0)
        return res;

    ctx->state = BME280_STATE_FORCE;
    ctx->reg = (uint8_t)BME280_REG_CTRL_MEAS;
    return bme280_force_measurement(ctx, m);
}

int bme280_read(struct bme280 *ctx, struct bme280_measurement *m)
{
    m->relative_humidity = 0;
    m->temperature = 0;
    m->pressure = 0;

    switch (ctx->state) {
    case BME280_STATE_SETUP: return bme280_read_setup(ctx, m);
    case BME280_STATE_SOFTRESET: return bme280_read_softreset(ctx, m);
    case BME280_STATE_TRIMMING_00: return bme280_read_trimming_00(ctx, m);
    case BME280_STATE_TRIMMING_26: return bme280_read_trimming_26(ctx, m);
    case BME280_STATE_SLEEP: return bme280_read_sleep(ctx, m);
    case BME280_STATE_FORCE: return bme280_force_measurement(ctx, m);
    case BME280_STATE_READ: return bme280_read_measurement(ctx, m);
    default: break;
    }

    picoRTOS_assert_void(false);
    /*@notreached@*/ return -EIO;
}
