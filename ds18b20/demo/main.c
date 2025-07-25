#include "picoRTOS.h"
#include "picoRTOS_device.h"

#include "board.h"
#include "ds18b20.h"

static bool needs_misting = false;

static void led_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL, return );

    struct gpio *L = (struct gpio*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    for (;;) {
        gpio_write(L, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(60ul));
        gpio_write(L, true);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(60ul));
        gpio_write(L, false);
        picoRTOS_sleep(PICORTOS_DELAY_MSEC(120ul));
        gpio_write(L, true);

        /* until next second */
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_SEC(1));
    }
}

static void sensor_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL, return );

    static struct ds18b20 ds18b20;
    struct w1 *W1 = (struct w1*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    (void) ds18b20(&ds18b20, W1);
    
    for (;;) {

      /* prepare new transaction */
      while(w1_reset(W1) < 0) picoRTOS_postpone();
      
      /* skip ROM */
      while(ds18b20_write(&ds18b20, DS18B20_ROM_SKIP, NULL, 0) < 0)
        picoRTOS_postpone();

      /* start measurement */
      while(ds18b20_read(&ds18b20, DS18B20_FN_CONVERT_T, NULL, 0) < 0)
        picoRTOS_postpone();
      
      picoRTOS_sleep_until(&ref, PICORTOS_DELAY_SEC(1));
    }
}

int main(void)
{
    static struct board board;

    struct picoRTOS_task task;
    static picoRTOS_stack_t stack0[CONFIG_DEFAULT_STACK_COUNT];
    static picoRTOS_stack_t stack1[CONFIG_DEFAULT_STACK_COUNT * 2];

    picoRTOS_init();
    (void)board_init(&board;

    /* led */
    picoRTOS_task_init(&task, led_main, board.L, stack0, PICORTOS_STACK_COUNT(stack0));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());
    /* 1-wire */
    picoRTOS_task_init(&task, sensor_main, board.W1, stack1, PICORTOS_STACK_COUNT(stack1));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());

    picoRTOS_start();

    /* not supposed to end there */
    picoRTOS_assert_void(false);
    return 1;
}
