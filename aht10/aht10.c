#include "aht10.h"
#include <errno.h>

#define AHT10_INIT_REQ         "\xe1\x08\x00"
#define AHT10_MEASURE_REQ      "\xac\x33\x00"
#define AHT10_STATUS_BUSY      (1 << 7)
#define AHT10_STATUS_CAL       (1 << 3)

#define AHT10_MEASURE_WAIT     PICORTOS_DELAY_MSEC(80l)
#define AHT10_MEASURE_RESP_LEN (1 + 5) /* status + data */

int aht10_init(struct aht10 *ctx, struct twi *i2c, twi_addr_t addr)
{
    ctx->i2c = i2c;
    ctx->addr = addr;

    ctx->state = AHT10_STATE_SETUP;
    ctx->len = 0;
    ctx->tick = 0;

    return 0;
}

void aht10_release(struct aht10 *ctx)
{
    /*@i@*/ (void)ctx;
}

static int aht10_read_measure_resp(struct aht10 *ctx, struct aht10_measurement *m)
{
    static char buf[AHT10_MEASURE_RESP_LEN];

    int res;
    size_t index = sizeof(buf) - ctx->len;

    if ((res = twi_read(ctx->i2c, &buf[index], ctx->len, TWI_F_START | TWI_F_STOP)) < 0)
        return res;

    if ((ctx->len -= (size_t)res) == 0) {
        /* rh */
        m->relative_humidity = (int)((((unsigned long)buf[1] << 12 |
                                       (unsigned long)buf[2] << 4 |
                                       (unsigned long)buf[3] >> 4) * 100ul) >> 20);
        /* t */
        m->temperature = (int)((((unsigned long)(buf[3] & 0xf) << 16 |
                                 (unsigned long)buf[4] << 8 |
                                 (unsigned long)buf[5]) * 200ul) >> 20) - 50;
        /* done */
        ctx->state = AHT10_STATE_SETUP;
        return (int)sizeof(*m);
    }

    return -EAGAIN;
}

static int aht10_calibration(struct aht10 *ctx)
{
    int res;
    size_t index = (sizeof(AHT10_INIT_REQ) - 1) - ctx->len;
    static const char req[] = { AHT10_INIT_REQ };

    if ((res = twi_write(ctx->i2c, &req[index], ctx->len, TWI_F_START | TWI_F_STOP)) < 0)
        return res;

    if ((ctx->len -= (size_t)res) == 0) {
        ctx->len = sizeof(AHT10_MEASURE_REQ) - 1;
        ctx->state = AHT10_STATE_MEASURE_REQ;
    }

    return -EAGAIN;
}

static int aht10_read_measure_status(struct aht10 *ctx, struct aht10_measurement *m)
{
    int res;
    char status = 0;

    if ((res = twi_read(ctx->i2c, &status, sizeof(status), TWI_F_START | TWI_F_STOP)) < 0)
        return res;

    if ((status & AHT10_STATUS_CAL) == 0) {
        /* calibration */
        ctx->len = sizeof(AHT10_INIT_REQ) - 1;
        ctx->state = AHT10_STATE_CALIBRATION;
        return aht10_calibration(ctx);
    }

    if ((status & AHT10_STATUS_BUSY) == 0) {
        /* response is ready */
        ctx->len = (size_t)AHT10_MEASURE_RESP_LEN;
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
    size_t index = (sizeof(AHT10_MEASURE_REQ) - 1) - ctx->len;
    static const char req[] = { AHT10_MEASURE_REQ };

    if ((res = twi_write(ctx->i2c, &req[index], ctx->len, TWI_F_START | TWI_F_STOP)) < 0)
        return res;

    if ((ctx->len -= (size_t)res) == 0) {
        ctx->state = AHT10_STATE_MEASURE_WAIT;
        ctx->tick = picoRTOS_get_tick();
        return aht10_read_measure_wait(ctx, m);
    }

    return -EAGAIN;
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

    ctx->state = AHT10_STATE_MEASURE_REQ;
    ctx->len = sizeof(AHT10_MEASURE_REQ) - 1;

    return aht10_read_measure_req(ctx, m);
}

int aht10_read(struct aht10 *ctx, struct aht10_measurement *m)
{
    m->relative_humidity = 0;
    m->temperature = 0;

    switch (ctx->state) {
    case AHT10_STATE_SETUP: return aht10_read_setup(ctx, m);
    case AHT10_STATE_MEASURE_REQ: return aht10_read_measure_req(ctx, m);
    case AHT10_STATE_MEASURE_WAIT: return aht10_read_measure_wait(ctx, m);
    case AHT10_STATE_MEASURE_STATUS: return aht10_read_measure_status(ctx, m);
    case AHT10_STATE_CALIBRATION: return aht10_calibration(ctx);
    case AHT10_STATE_MEASURE_RESP: return aht10_read_measure_resp(ctx, m);
    default: break;
    }

    picoRTOS_break();
    /*@notreached@*/ return -EIO;
}
