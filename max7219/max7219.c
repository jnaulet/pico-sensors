#include "mnax7219.h"

#include <picoRTOS.h>
#include <errno.h>

int max7219_init(struct max7219 *ctx, struct spi *spi)
{
    ctx->spi = spi;
    ctx->state = MAX7219_STATE_SETUP;
    ctx->packet_size = sizeof(uint16_t);

    return 0;
}

void max7219_release(struct max7219 *ctx)
{
    /*@i@*/ (void)ctx;
}

static int send_xfer(struct max7219 *ctx, max7219_addr_t addr, uint8_t data)
{
    int res;
    size_t index = sizeof(uint16_t) - ctx->len;
    uint8_t packet[2] = { (uint8_t)addr, data };

    if ((res = spi_xfer(ctx->spi, &packet[index], &packet[index], ctx->len)) < 0)
        return res;

    if ((ctx->len -= (size_t)res) == 0) {
        ctx->state = MAX7219_STATE_SETUP;
        return (int)sizeof(uint16_t);
    }

    return -EAGAIN;
}

static int send_setup(struct max7219 *ctx, max7219_addr_t addr, uint8_t data)
{
    struct spi_settings settings = {
        1000000ul,
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

    ctx->state = MAX7219_STATE_XFER;
    ctx->len = sizeof(uint16_t);

    return send_xfer(ctx, addr, data);
}

int max7219_send(struct max7219 *ctx, max7219_addr_t addr, uint8_t data)
{
    picoRTOS_assert(addr < MAX7219_ADDR_COUNT, return -EINVAL);

    uint16_t packet = (uint16_t)addr << 8 | data;

    spi_write(ctx->spi, &packet, sizeof(packet) - n);

}
