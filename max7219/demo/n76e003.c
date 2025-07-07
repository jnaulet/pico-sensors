#include "board.h"
#include "picoRTOS_device.h"

#include "clock-numicro.h"
#include "mux-n76e003.h"

#include "gpio-n76e003.h"
#include "spi-n76e003.h"

static void clock_init(void)
{
    struct clock_settings CLOCK_settings = {
        CLOCK_NUMICRO_FOSC_FHIRC,
        CLOCK_NUMICRO_FHIRC_16_0MHZ,
        0ul,    /* feclk */
        1u,     /* clkdiv */
    };

    (void)clock_numicro_init(&CLOCK_settings);
}

static void mux_init(void)
{
    struct mux P0;
    struct mux P1;

    (void)mux_n76e003_init(&P0, (size_t)0);
    (void)mux_n76e003_init(&P1, (size_t)1);

    (void)mux_n76e003_mode(&P0, (size_t)0, MUX_N76E003_MODE_QUASIBDIR); /* MOSI */
    (void)mux_n76e003_mode(&P1, (size_t)0, MUX_N76E003_MODE_QUASIBDIR); /* SPCLK */
    (void)mux_n76e003_mode(&P1, (size_t)1, MUX_N76E003_MODE_QUASIBDIR); /* LOAD */
    (void)mux_n76e003_mode(&P1, (size_t)5, MUX_N76E003_MODE_QUASIBDIR); /* L */
}

static int gpio_init(/*@partial@*/ struct board *ctx)
{
    static struct gpio LOAD;
    static struct gpio L;

    (void)gpio_n76e003_init(&LOAD, (size_t)1, (size_t)1);
    (void)gpio_n76e003_init(&L, (size_t)1, (size_t)5);

    (void)gpio_write(&LOAD, true);

    ctx->LOAD = &LOAD;
    ctx->L = &L;
    return 0;
}

static int spi_init(/*@partial@*/ struct board *ctx)
{
    static struct spi SPI;
    struct spi_settings SPI_settings = {
        1000000ul,
        SPI_MODE_MASTER,
        SPI_CLOCK_MODE_0,
        (size_t)8,
        SPI_CS_POL_ACTIVE_LOW,
        (size_t)0
    };

    (void)spi_n76e003_init(&SPI, CLOCK_NUMICRO_FSYS);
    (void)spi_setup(&SPI, &SPI_settings);

    ctx->SPI = &SPI;
    return 0;
}

int board_init(struct board *ctx)
{
    /* clocks */
    clock_init();
    mux_init();

    /* peripherals */
    (void)gpio_init(ctx);
    (void)spi_init(ctx);

    return 0;
}
