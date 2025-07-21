#include "ht16k33.h"
#include <errno.h>

int ht16k33_init(struct ht16k33 *ctx, struct twi *i2c, twi_addr_t addr)
{
    picoRTOS_assert(addr >= HT16K33_BASEADDR, return -EINVAL);
    picoRTOS_assert((int)addr < (HT16K33_BASEADDR + HT16K33_BASEADDR_COUNT), return -EINVAL);

    ctx->i2c = i2c;
    ctx->addr = addr;
    ctx->state = HT16K33_STATE_SETUP;

    return 0;
}

void ht16k33_release(struct ht16k33 *ctx)
{
    /*@i@*/ (void)ctx;
}

static int ht16k33_setup(struct ht16k33 *ctx)
{
    int res;
    struct twi_settings i2c_settings = {
        TWI_BITRATE_STANDARD,
        TWI_MODE_MASTER,
        ctx->addr
    };

    if ((res = twi_setup(ctx->i2c, &i2c_settings)) < 0)
        return res;

    ctx->state = HT16K33_STATE_CC;
    return -EAGAIN;
}

static int ht16k33_write_data(struct ht16k33 *ctx, /*@null@*/ const void *buf, size_t n)
{
    picoRTOS_assert(buf != NULL, return -EINVAL);
    picoRTOS_assert(n > 0, return -EINVAL);

    int res;

    if ((res = twi_write(ctx->i2c, buf, n, TWI_F_P)) == (int)n)
        ctx->state = HT16K33_STATE_SETUP;

    return res;
}

static int ht16k33_write_cc(struct ht16k33 *ctx, uint8_t cc, /*@null@*/ const void *buf, size_t n)
{
    int res = -EAGAIN;

    if (buf != NULL) {
        /* +n to ensure no repeated start is emitted */
        if ((res = twi_write(ctx->i2c, &cc, sizeof(cc) + n, TWI_F_S | TWI_F_N(1))) == 1) {
            ctx->state = HT16K33_STATE_DATA;
            return ht16k33_write_data(ctx, buf, n);
        }

    } else if ((res = twi_write(ctx->i2c, &cc, sizeof(cc), TWI_F_S | TWI_F_P)) == 1)
        ctx->state = HT16K33_STATE_SETUP;

    return res;
}

int ht16k33_write(struct ht16k33 *ctx, int cc, const void *buf, size_t n)
{
    switch (ctx->state) {
    case HT16K33_STATE_SETUP: return ht16k33_setup(ctx);
    case HT16K33_STATE_CC: return ht16k33_write_cc(ctx, (uint8_t)cc, buf, n);
    case HT16K33_STATE_DATA: return ht16k33_write_data(ctx, buf, n);
    default: break;
    }

    picoRTOS_break();
    /*@notreached@*/ return -EIO;
}

static int ht16k33_read_data(struct ht16k33 *ctx, void *buf, size_t n)
{
    picoRTOS_assert(n > 0, return -EINVAL);

    int res;

    /* repeated start + read */
    if ((res = twi_read(ctx->i2c, buf, n, TWI_F_S | TWI_F_P)) == (int)n)
        ctx->state = HT16K33_STATE_SETUP;

    return res;
}

static int ht16k33_read_cc(struct ht16k33 *ctx, uint8_t cc, void *buf, size_t n)
{
    picoRTOS_assert(n > 0, return -EINVAL);

    int res;

    if ((res = twi_write(ctx->i2c, &cc, sizeof(cc), TWI_F_S)) < 0)
        return res;

    ctx->state = HT16K33_STATE_DATA;
    return ht16k33_read_data(ctx, buf, n);
}

int ht16k33_read(struct ht16k33 *ctx, int cc, void *buf, size_t n)
{
    switch (ctx->state) {
    case HT16K33_STATE_SETUP: return ht16k33_setup(ctx);
    case HT16K33_STATE_CC: return ht16k33_read_cc(ctx, (uint8_t)cc, buf, n);
    case HT16K33_STATE_DATA: return ht16k33_read_data(ctx, buf, n);
    default: break;
    }

    picoRTOS_break();
    /*@notreached@*/ return -EIO;
}
