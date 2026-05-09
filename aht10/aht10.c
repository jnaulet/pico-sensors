#include "aht10.h"
#include <errno.h>

#define AHT10_SOFTRESET_REQ    "\xba"
#define AHT10_INIT_REQ         "\xe1\x08\x00"
#define AHT10_MEASURE_REQ      "\xac\x33\x00"
#define AHT10_STATUS_BUSY      (1 << 7)
#define AHT10_STATUS_CAL       (1 << 3)

#define AHT10_SOFTRESET_WAIT   PICORTOS_DELAY_MSEC(20l)
#define AHT10_MEASURE_WAIT     PICORTOS_DELAY_MSEC(80l)
#define AHT10_MEASURE_RESP_LEN (1 + 5) /* status + data */

int aht10_init(struct aht10 *ctx, struct twi *i2c, twi_addr_t addr)
{
    ctx->i2c = i2c;
    ctx->addr = addr;

    ctx->state = AHT10_STATE_SETUP;
    ctx->tick = 0;

    ctx->xfer.n = 0;
    ctx->xfer.total = 0;

    return 0;
}

void aht10_release(struct aht10 *ctx)
{
    /*@i@*/ (void)ctx;
}

static int twi_write_req(struct aht10 *ctx, const char *req, size_t n)
{
    if (ctx->xfer.total == 0) {
        ctx->xfer.total = n;
        ctx->xfer.n = 0;
    }

    int res;
    if ((res = twi_write(ctx->i2c, &req[ctx->xfer.n],
                         ctx->xfer.total - ctx->xfer.n,
                         TWI_F_S | TWI_F_P)) < 0)
        return res;

    ctx->xfer.n += res;
    if (ctx->xfer.n == (int)ctx->xfer.total) {
        ctx->xfer.total = 0;
        return ctx->xfer.n;
    }

    return -EAGAIN;
}

static int twi_read_req(struct aht10 *ctx, uint8_t *buf, size_t n)
{
    if (ctx->xfer.total == 0) {
        ctx->xfer.total = n;
        ctx->xfer.n = 0;
    }

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

    return -EAGAIN;
}

static int aht10_read_measure_resp(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;
    static uint8_t buf[AHT10_MEASURE_RESP_LEN];

    if ((res = twi_read_req(ctx, buf, sizeof(buf))) < 0)
        return res;

    /* rh */
    m->relative_humidity = (int)((((unsigned long)buf[1] << 12 |
                                   (unsigned long)buf[2] << 4 |
                                   (unsigned long)buf[3] >> 4) * 100ul) >> 20);
    /* t */
    m->temperature = (int)((((unsigned long)(buf[3] & 0xf) << 16 |
                             (unsigned long)buf[4] << 8 |
                             (unsigned long)buf[5]) * 200ul) >> 20) - 50;

    /* done */
    ctx->state = AHT10_STATE_SLEEP;
    return (int)sizeof(*m);
}

#ifndef AHT10_CONFIG_NO_CALIBRATION
static int aht10_calibration(struct aht10 *ctx)
{
    int res;

    if ((res = twi_write_req(ctx, AHT10_INIT_REQ,
                             sizeof(AHT10_INIT_REQ) - 1)) < 0)
        return res;

    ctx->state = AHT10_STATE_MEASURE_REQ;
    return -EAGAIN;
}
#endif

static int aht10_read_measure_status(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;
    char status = (char)0;

    if ((res = twi_read(ctx->i2c, &status, sizeof(status), TWI_F_S)) < 0)
        return res;

    if (((int)status & AHT10_STATUS_CAL) == 0) {
#ifndef AHT10_CONFIG_NO_CALIBRATION
        /* calibration */
        ctx->state = AHT10_STATE_CALIBRATION;
        return aht10_calibration(ctx);
#else
        picoRTOS_assert_void(false);
        /*@notreached@*/ return -EFAULT;
#endif
    }

    if (((int)status & AHT10_STATUS_BUSY) == 0) {
        /* response is ready */
        ctx->state = AHT10_STATE_MEASURE_RESP;
        return aht10_read_measure_resp(ctx, m);
    }else{
        /* retry */
        ctx->state = AHT10_STATE_MEASURE_WAIT;
        ctx->tick = picoRTOS_get_tick();
    }

    return -EAGAIN;
}

static int aht10_read_measure_wait(struct aht10 *ctx, struct aht10_measurement *m)
{
    if ((picoRTOS_get_tick() - ctx->tick) >= AHT10_MEASURE_WAIT) {
        ctx->state = AHT10_STATE_MEASURE_STATUS;
        return aht10_read_measure_status(ctx, m);
    }

    return -EAGAIN;
}

static int aht10_read_measure_req(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;

    if ((res = twi_write_req(ctx, AHT10_MEASURE_REQ,
                             sizeof(AHT10_MEASURE_REQ) - 1)) < 0)
        return res;

    ctx->tick = picoRTOS_get_tick();
    ctx->state = AHT10_STATE_MEASURE_WAIT;
    return aht10_read_measure_wait(ctx, m);
}

static int aht10_read_softreset_wait(struct aht10 *ctx, struct aht10_measurement *m)
{
    if ((picoRTOS_get_tick() - ctx->tick) >= AHT10_SOFTRESET_WAIT) {
        ctx->state = AHT10_STATE_MEASURE_REQ;
        return aht10_read_measure_req(ctx, m);
    }

    return -EAGAIN;
}

static int aht10_read_softreset(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;

    if ((res = twi_write_req(ctx, AHT10_SOFTRESET_REQ,
                             sizeof(AHT10_SOFTRESET_REQ) - 1)) < 0)
        return res;

    ctx->tick = picoRTOS_get_tick();
    ctx->state = AHT10_STATE_SOFTRESET_WAIT;
    return aht10_read_softreset_wait(ctx, m);
}

static int aht10_read_setup(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;
    struct twi_settings i2c_settings = {
        TWI_BITRATE_STANDARD,
        TWI_MODE_MASTER,
        ctx->addr
    };

    if ((res = twi_setup(ctx->i2c, &i2c_settings)) < 0)
        return res;

    ctx->state = AHT10_STATE_SOFTRESET;
    return aht10_read_softreset(ctx, m);
}

static int aht10_read_sleep(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;
    struct twi_settings i2c_settings = {
        TWI_BITRATE_STANDARD,
        TWI_MODE_MASTER,
        ctx->addr
    };

    if ((res = twi_setup(ctx->i2c, &i2c_settings)) < 0)
        return res;

    ctx->state = AHT10_STATE_MEASURE_REQ;
    return aht10_read_measure_req(ctx, m);
}

int aht10_read(struct aht10 *ctx, struct aht10_measurement *m)
{
    m->relative_humidity = 0;
    m->temperature = 0;

    switch (ctx->state) {
    case AHT10_STATE_SETUP: return aht10_read_setup(ctx, m);
    case AHT10_STATE_SOFTRESET: return aht10_read_softreset(ctx, m);
    case AHT10_STATE_SOFTRESET_WAIT: return aht10_read_softreset_wait(ctx, m);
    case AHT10_STATE_SLEEP: return aht10_read_sleep(ctx, m);
    case AHT10_STATE_MEASURE_REQ: return aht10_read_measure_req(ctx, m);
    case AHT10_STATE_MEASURE_WAIT: return aht10_read_measure_wait(ctx, m);
    case AHT10_STATE_MEASURE_STATUS: return aht10_read_measure_status(ctx, m);
#ifndef AHT10_CONFIG_NO_CALIBRATION
    case AHT10_STATE_CALIBRATION: return aht10_calibration(ctx);
#endif
    case AHT10_STATE_MEASURE_RESP: return aht10_read_measure_resp(ctx, m);
    default: break;
    }

    picoRTOS_assert_void(false);
    /*@notreached@*/ return -EIO;
}
