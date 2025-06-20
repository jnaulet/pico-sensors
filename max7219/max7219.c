#include "mnax7219.h"

int max7219_init(struct max7219 *ctx, struct spi *spi)
{
    ctx->spi = spi;
    return 0;
}

void max7219_release(struct max7219 *ctx)
{
    /*@i@*/ (void)ctx;
}

