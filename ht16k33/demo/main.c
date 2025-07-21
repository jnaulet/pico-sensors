#include "picoRTOS.h"
#include "picoRTOS_device.h"

#include "board.h"
#include "ht16k33.h"

#include <string.h>

static void led_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL, return );

    struct gpio *L = (struct gpio*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    for (;;) {
        gpio_write(L, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(40ul));
        gpio_write(L, true);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(40ul));
        gpio_write(L, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(120ul));
        gpio_write(L, true);

        /* until next round */
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_SEC(1));
    }
}

static int ht16k33_write_wait(struct ht16k33 *ctx, uint8_t cc, /*@null@*/ const void *buf, size_t n)
{
  int nwritten = 0;
  uint8_t *buf8 = (uint8_t*) buf;
  int deadlock = CONFIG_DEADLOCK_COUNT;

  while(nwritten != (int)n && deadlock-- != 0){
    int res;
    if((res = ht16k33_write(ctx, cc, buf, n)) < 0) picoRTOS_postpone();
    else nwritten += res;
  }

  picoRTOS_assert_fatal(deadlock != -1, return -ETIMEDOUT);
  return (int)n;
}

static void display_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL, return );

    struct board *ctx = (struct board*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    static struct ht16k33 ht16k33;
    (void) ht16k33_init(&ht16k33, ctx->I2C, HT16K33_BASEADDR);

    static uint8_t zero[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0 };

    /* init display */
    (void) ht16k33_write_wait(&ht16k33, (uint8_t)HT16K33_CC_DISPLAY_DATA, zero, sizeof(zero));
    (void) ht16k33_write_wait(&ht16k33, (uint8_t)(HT16K33_CC_SYSTEM_SETUP|S), NULL, 0); /* osc on */
    (void) ht16k33_write_wait(&ht16k33, (uint8_t)(HT16K33_CC_DISPLAY_SETUP|D), NULL, 0); /* display on */

    for(;;){
      picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(100ul));
    }
}

int main(void)
{
    static struct board board;

    struct picoRTOS_task task;
    static picoRTOS_stack_t stack0[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack1[CONFIG_DEFAULT_STACK_COUNT * 2];

    picoRTOS_init();
    (void)board_init(&board);

    /* led */
    picoRTOS_task_init(&task, led_main, board.L, stack0, PICORTOS_STACK_COUNT(stack0));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    /* display */
    picoRTOS_task_init(&task, display_main, &board, stack1, PICORTOS_STACK_COUNT(stack1));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());

    picoRTOS_start();

    /* not supposed to end there */
    picoRTOS_assert_void(false);
    return 1;
}
