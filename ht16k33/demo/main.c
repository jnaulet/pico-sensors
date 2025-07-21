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
    int deadlock = CONFIG_DEADLOCK_COUNT;

    if (buf == NULL) {
        while (ht16k33_write(ctx, cc, NULL, 0) < 0 && deadlock-- != 0)
            picoRTOS_postpone();

        return 0;
    }

    int nwritten = 0;
    uint8_t *buf8 = (uint8_t*)buf;

    while (nwritten != (int)n && deadlock-- != 0) {
        int res;
        if ((res = ht16k33_write(ctx, cc, &buf8[nwritten], n - (size_t)nwritten)) < 0) {
            picoRTOS_postpone();
            continue;
        }

        nwritten += res;
    }

    picoRTOS_assert_fatal(deadlock != -1, return -EBUSY);
    return (int)n;
}

#define S     (uint16_t)0xedu
#define O     (uint16_t)0x3fu
#define U     (uint16_t)0x3eu
#define N     (uint16_t)0x2136u
#define Y     (uint16_t)0x1500u
#define LT    (uint16_t)0x2400u
#define THREE (uint16_t)0x8fu

static void display_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL, return );

    struct board *ctx = (struct board*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    static uint16_t zero[4];
    static struct ht16k33 ht16k33;

    (void)ht16k33_init(&ht16k33, ctx->I2C, HT16K33_BASEADDR);

    /* init display */
    (void)ht16k33_write_wait(&ht16k33, HT16K33_CC_DISPLAY_DATA, zero, sizeof(zero));
    (void)ht16k33_write_wait(&ht16k33, HT16K33_CC_SYSTEM_SETUP | HT16K33_S, NULL, 0);   /* osc on */
    (void)ht16k33_write_wait(&ht16k33, HT16K33_CC_ROW_INT_SET, NULL, 0);                /* row mode */
    (void)ht16k33_write_wait(&ht16k33, HT16K33_CC_DISPLAY_SETUP | HT16K33_D, NULL, 0);  /* display on */
    (void)ht16k33_write_wait(&ht16k33, HT16K33_CC_DIMMING_SET | HT16K33_P(0x4), NULL, 0);

    for (;;) {

        static int i = 0;
        static uint16_t fb[] = { 0, 0,  0,     0,
                                 S, O,  U,     N,
                                 Y, LT, THREE, 0,
                                 0, 0,  0,     0 };
#if 1
        (void)ht16k33_write_wait(&ht16k33, (uint8_t)HT16K33_CC_DISPLAY_DATA, &fb[i], (size_t)8);
        if (++i == 12) i = 0; /* reset scroll */
#else
        uint16_t data = (uint16_t)(1 << i);
        (void)ht16k33_write_wait(&ht16k33, (uint8_t)HT16K33_CC_DISPLAY_DATA, &data, sizeof(data));
        if (++i == 16) i = 0; /* reset scroll */
#endif
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
