#include "picoRTOS.h"
#include "picoRTOS_device.h"

#include "board.h"
#include "aht10.h"

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

static void sensors_dbg(struct uart *DBG, int t, int rh)
{
    int sent = 0;
    static char dbg[] = { "xxC - xx%\n" };

    static const char hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', //
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    dbg[0] = hex[t / 10];
    dbg[1] = hex[t % 10];
    /* °C - */
    dbg[6] = hex[rh / 10];
    dbg[7] = hex[rh % 10];
    /* %\n */

    while (sent != sizeof(dbg) - 1) {
        int n = uart_write(DBG, &dbg[sent], sizeof(dbg) - sent);
        if (n < 0) picoRTOS_postpone();
        else sent += n;
    }
}

static void rh_t_main(void *priv)
{
#define TEMP_CORRECTION -5 /* The chip is very optimistic */
    picoRTOS_assert_fatal(priv != NULL, return );

    struct board *ctx = (struct board*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    static struct aht10 aht10;
    (void)aht10_init(&aht10, ctx->I2C, AHT10_DEFAULT_ADDR);

    /* sensor needs a >= 100ms delay */
    picoRTOS_sleep(PICORTOS_DELAY_MSEC(120l));

    for (;;) {

        int res;
        struct aht10_measurement m;
        int deadlock = CONFIG_DEADLOCK_COUNT;

        /* AHT10 */
        while ((res = aht10_read(&aht10, &m)) < 0 && deadlock-- != 0) {
            /* reduce power consumption */
            if (aht10.state == AHT10_STATE_MEASURE_WAIT) picoRTOS_sleep(PICORTOS_DELAY_MSEC(80l));
            else picoRTOS_postpone();
        }

        if (deadlock < 0 || res != (int)sizeof(m))
            continue;

        sensors_dbg(ctx->DBG, m.temperature, m.relative_humidity);
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_SEC(2));
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
    /* rh_t */
    picoRTOS_task_init(&task, rh_t_main, &board, stack1, PICORTOS_STACK_COUNT(stack1));
    picoRTOS_add_task(&task, picoRTOS_get_next_available_priority());

    picoRTOS_start();

    /* not supposed to end there */
    picoRTOS_assert_void(false);
    return 1;
}
