#include "board.h"
#include "picoRTOS_device.h"

#include "clock-n76e003.h"
#include "mux-n76e003.h"
#include "gpio-n76e003.h"
#include "twi-n76e003.h"

static void clock_init(void)
{
    struct clock_settings CLOCK_settings = {
        CLOCK_N76E003_FOSC_FHIRC,
        CLOCK_N76E003_FHIRC_16_0MHZ,
        0ul,    /* feclk */
        1u,     /* clkdiv */
    };

    (void)clock_n76e003_init(&CLOCK_settings);
}

static void mux_init(void)
{
    struct mux P1;

    (void)mux_n76e003_init(&P1, (size_t)1);

    (void)mux_n76e003_mode(&P1, (size_t)3, MUX_N76E003_MODE_OPENDRAIN); /* SCL */
    (void)mux_n76e003_mode(&P1, (size_t)4, MUX_N76E003_MODE_OPENDRAIN); /* SDA */
    (void)mux_n76e003_mode(&P1, (size_t)5, MUX_N76E003_MODE_QUASIBDIR); /* L */
}

static int gpio_init(/*@partial@*/ struct board *ctx)
{
    static struct gpio L;

    (void)gpio_n76e003_init(&L, (size_t)1, (size_t)5);

    ctx->L = &L;
    return 0;
}

static int i2c_init(/*@partial@*/ struct board *ctx)
{
    static struct twi I2C;

    /* pins 1.3 & 1.4 (i2cpx = false) */
    (void)twi_n76e003_init(&I2C, CLOCK_N76E003_FSYS, false);

    ctx->I2C = &I2C;
    return 0;
}

int board_init(struct board *ctx)
{
    /* clocks */
    clock_init();
    mux_init();

    /* peripherals */
    (void)gpio_init(ctx);
    (void)i2c_init(ctx);

    return 0;
}
