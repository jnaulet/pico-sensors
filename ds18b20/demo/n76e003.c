#include "board.h"
#include "picoRTOS_device.h"

#include "clock-n76e003.h"
#include "mux-n76e003.h"

#include "gpio-n76e003.h"
#include "uart-n76e003.h"
#include "w1-uart.h"

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
    struct mux P0;
    struct mux P1;

    (void)mux_n76e003_init(&P0, (size_t)0);
    (void)mux_n76e003_init(&P1, (size_t)1);

    (void)mux_n76e003_mode(&P0, (size_t)6, MUX_N76E003_MODE_QUASIBDIR); /* TXD */
    (void)mux_n76e003_mode(&P0, (size_t)7, MUX_N76E003_MODE_QUASIBDIR); /* RXD */
    (void)mux_n76e003_mode(&P1, (size_t)5, MUX_N76E003_MODE_QUASIBDIR); /* L */
}

static int gpio_init(/*@partial@*/ struct board *ctx)
{
  static struct gpio L;
  (void)gpio_n76e003_init(&L, (size_t)1, (size_t)5);

  ctx->L = &L;
  return 0;
}

static int w1_init(/*@partial@*/ struct n76e003 *ctx)
{
  static struct uart UART0;
  static struct w1 W1;

  (void)uart_n76e003_init(&UART0, UART_N76E003_UART0_T1);
  (void)w1_uart_init(&W1, &UART0);

  ctx->W1 = &W1;
  return 0;
}

int board_init(struct board *ctx)
{
    /* clocks */
    clock_init();
    mux_init();

    /* peripherals */
    (void)gpio_init(ctx);
    (void)w1_init(ctx);

    return 0;
}
