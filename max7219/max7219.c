#include "max7219.h"

#include <picoRTOS.h>
#include <errno.h>

int max7219_init(struct max7219 *ctx, struct spi *spi, struct gpio *load)
{
    ctx->spi = spi;
    ctx->load = load;
    ctx->state = MAX7219_STATE_SETUP;
    ctx->pos = 0;

    return 0;
}

void max7219_release(struct max7219 *ctx)
{
    /*@i@*/ (void)ctx;
}

static int send_xfer(struct max7219 *ctx, max7219_addr_t addr, const uint8_t *data, size_t n)
{
    picoRTOS_assert(addr < MAX7219_ADDR_COUNT, return -EINVAL);
    picoRTOS_assert(n > 0, return -EINVAL);

    int xfered = 0;

    while (xfered < (int)n) {

        int res;
        size_t len = sizeof(uint16_t) - (size_t)ctx->pos;
        /* probably not worth the trouble of computing once */
        uint8_t packet[2] = { (uint8_t)addr, data[xfered] };

        if ((res = spi_xfer(ctx->spi, packet, &packet[ctx->pos], len)) < 0)
            break;

        ctx->pos = (ctx->pos + res) & 1;
        if (ctx->pos == 0)
            xfered++;
    }

    if (xfered == 0)
        return -EAGAIN;

    if (xfered == (int)n) {
        gpio_write(ctx->load, true);
        ctx->state = MAX7219_STATE_SETUP;
    }

    return xfered;
}

static int send_setup(struct max7219 *ctx, max7219_addr_t addr, const uint8_t *data, size_t n)
{
    picoRTOS_assert(addr < MAX7219_ADDR_COUNT, return -EINVAL);
    picoRTOS_assert(n > 0, return -EINVAL);

    int res;
    struct spi_settings settings = {
        0ul,
        SPI_MODE_MASTER,
        SPI_CLOCK_MODE_0,
        (size_t)16,
        SPI_CS_POL_ACTIVE_LOW,
        (size_t)0
    };

    if ((res = spi_setup(ctx->spi, &settings)) < 0) {
        /* attempt a 8bit only */
        settings.frame_size = (size_t)8;
        if ((res = spi_setup(ctx->spi, &settings)) < 0)
            return res;
    }

    gpio_write(ctx->load, false);
    ctx->state = MAX7219_STATE_XFER;
    ctx->pos = 0;

    return send_xfer(ctx, addr, data, n);
}

int max7219_send(struct max7219 *ctx, max7219_addr_t addr, const uint8_t *data, size_t n)
{
    picoRTOS_assert(addr < MAX7219_ADDR_COUNT, return -EINVAL);
    picoRTOS_assert(n > 0, return -EINVAL);

    switch (ctx->state) {
    case MAX7219_STATE_SETUP: return send_setup(ctx, addr, data, n);
    case MAX7219_STATE_XFER: return send_xfer(ctx, addr, data, n);
    default: break;
    }

    picoRTOS_break();
    /*@notreached@*/ return -EIO;
}
