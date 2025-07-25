#include "ds18b20.h"

int ds18b20_init(struct ds18b20 *ctx, struct w1 *w1)
{
    ctx->w1 = w1;
    ctx->state = DS18B20_STATE_CMD;

    return 0;
}

void ds18b20_release(struct ds18b20 *ctx)
{
    /*@i@*/ (void)ctx;
}

int ds18b20_write(struct ds18b20 *ctx, int cmd, /*@null@*/ const void *buf, size_t n)
{
    int res;

    if (ctx->state == DS18B20_STATE_CMD) {
        uint8_t c = (uint8_t)cmd;
        if ((res = w1_write(ctx->w1, &c, sizeof(c))) < 0)
            return res;

        if (buf != NULL && n > 0) ctx->state = DS18B20_STATE_DATA;
        else return 0;
    }

    if (ctx->state == DS18B20_STATE_DATA) {
        /* send data */
        if ((res = w1_write(ctx->w1, buf, n)) < 0)
            return res;

        if (res == (int)n)
            ctx->state = DS18B20_STATE_CMD;

        return res;
    }

    picoRTOS_break();
    /*@notreached@*/ return -EIO;
}

int ds18b20_read(struct ds18b20 *ctx, int cmd, /*@null@*/ void *buf, size_t n)
{
    int res;

    if (ctx->state == DS18B20_STATE_CMD) {
        uint8_t c = (uint8_t)cmd;
        if ((res = w1_write(ctx->w1, &c, sizeof(c))) < 0)
            return res;

        if (buf != NULL && n > 0) ctx->state = DS18B20_STATE_DATA;
        else return 0;
    }

    if (ctx->state == DS18B20_STATE_DATA) {
        if ((res = w1_read(ctx->w1, buf, n)) < 0)
            return res;

        if (res == (int)n)
            ctx->state = DS18B20_STATE_CMD;

        return res;
    }

    picoRTOS_break();
    /*@notreached@*/ return -EIO;
}
