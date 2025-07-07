#include "board.h"
#include "picoRTOS_device.h"

#include "clock-lgt8fx8p.h"
#include "mux-avr.h"

#include "gpio-avr.h"
#include "spi-lgt8fx8p.h"

static void clock_init(void)
{
    struct clock_settings CLOCK_settings = {
        0ul,
        CLOCK_LGT8FX8P_MCLK_HFRC,
        CLOCK_LGT8FX8P_PS_2,
        CLOCK_LGT8FX8P_WDCLKS_LFRC
    };

    (void)clock_lgt8fx8p_init(&CLOCK_settings);
}

static void mux_init(void)
{
    struct mux PORTB;
    struct mux PORTE;

    (void)mux_avr_init(&PORTB, ADDR_PORTB);
    (void)mux_avr_init(&PORTE, ADDR_PORTE);

    (void)mux_avr_output(&PORTB, (size_t)5, false); /* SCK */
    (void)mux_avr_input(&PORTB, (size_t)4);         /* MISO */
    (void)mux_avr_output(&PORTB, (size_t)3, false); /* MOSI */
    (void)mux_avr_output(&PORTB, (size_t)2, true);  /* _SS */

    (void)mux_avr_output(&PORTE, (size_t)3, true);  /* L */
}

static int gpio_init(/*@partial@*/ struct board *ctx)
{
    static struct gpio LOAD;
    static struct gpio L;

    (void)gpio_avr_init(&LOAD, ADDR_PORTB, (size_t)2);
    (void)gpio_avr_init(&L, ADDR_PORTE, (size_t)3);

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

    (void)spi_lgt8fx8p_init(&SPI, ADDR_SPI, CLOCK_LGT8FX8P_PERI_CLK);
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
