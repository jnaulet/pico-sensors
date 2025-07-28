#include "board.h"
#include "picoRTOS_device.h"

#include "clock-lgt8fx8p.h"
#include "mux-avr.h"

#include "gpio-avr.h"
#include "spi-lgt8fx8p.h"
#include "twi-avr.h"
#include "adc-avr.h"
#include "uart-avr.h"

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
    struct mux PORTC;
    struct mux PORTE;

    (void)mux_avr_init(&PORTC, ADDR_PORTC);
    (void)mux_avr_init(&PORTE, ADDR_PORTE);


    (void)mux_avr_output(&PORTE, (size_t)3, true);  /* L */
    (void)mux_avr_output(&PORTC, (size_t)5, false); /* SCL */
    (void)mux_avr_output(&PORTC, (size_t)4, false); /* SDA */
}

static int gpio_init(/*@partial@*/ struct board *ctx)
{
    static struct gpio L;

    (void)gpio_avr_init(&L, ADDR_PORTE, (size_t)3);

    ctx->L = &L;
    return 0;
}

static int i2c_init(/*@partial@*/ struct board *ctx)
{
    static struct twi I2C;
    struct twi_settings TWI_settings = {
        TWI_BITRATE_STANDARD,
        TWI_MODE_MASTER,
        (twi_addr_t)0x38
    };

    (void)twi_avr_init(&I2C, ADDR_TWI, CLOCK_LGT8FX8P_PERI_CLK);
    (void)twi_setup(&I2C, &TWI_settings);

    ctx->I2C = &I2C;
    return 0;
}

static int uart_init(/*@partial@*/ struct board *ctx)
{
    static struct uart USART;
    const struct uart_settings UART_settings = {
        9600ul,
        (size_t)8,
        UART_PAR_NONE,
        UART_CSTOPB_1BIT,
    };

    (void)uart_avr_init(&USART, ADDR_USART, CLOCK_LGT8FX8P_PERI_CLK);
    (void)uart_setup(&USART, &UART_settings);

    ctx->DBG = &USART;
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
    (void)uart_init(ctx);

    return 0;
}
