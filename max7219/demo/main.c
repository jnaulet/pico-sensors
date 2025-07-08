#include "picoRTOS.h"
#include "picoRTOS_device.h"

#include "board.h"
#include "max7219.h"

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

static int max7219_send_wait(struct max7219 *ctx, max7219_addr_t addr,
                             const uint8_t *data, size_t n)
{
    picoRTOS_assert(addr < MAX7219_ADDR_COUNT, return -EINVAL);
    picoRTOS_assert(n > 0, return -EINVAL);

    int i = 0;
    int deadlock = CONFIG_DEADLOCK_COUNT;

    while (i < (int)n) {
        int res;
        if ((res = max7219_send(ctx, addr, &data[i], n - (size_t)i)) < 0 && deadlock-- != 0) {
            picoRTOS_postpone();
            continue;
        }

        picoRTOS_assert(deadlock != 0, return -EBUSY);
        i += res;
    }

    return i;
}

/*
 * framebuffer
 */
#define DIGIT_COUNT  4
#define LINE_COUNT   8
#define SPRITE_COUNT 12

struct fb {
    uint8_t buf[DIGIT_COUNT * LINE_COUNT];
    struct {
        uint_fast8_t x, y;
        uint32_t data;
    } sprite[SPRITE_COUNT];
};

#define SPRITE_SET(s, _x, _y, _data)                                  \
    do {                                                                \
        (s)->x = (uint_fast8_t)_x;                                        \
        (s)->y = (uint_fast8_t)_y;                                        \
        (s)->data = (uint32_t)_data;                                      \
    } while(false)

static void fb_set(struct fb *fb, uint8_t val)
{
    memset(fb, (int)val, sizeof(*fb));
}

static void fb_invert(struct fb *fb)
{
    for (uint_fast8_t n = (uint_fast8_t)sizeof(fb->buf); n-- != 0;)
        fb->buf[n] = ~fb->buf[n];
}

#define C      0x788870
#define HEARTL 0x348960
#define HEARTR 0x8422c0
#define N      0x99bd90
#define O      0x699960
#define S      0xe16870
#define U      0x699990
#define Y      0x226990

static void fb_render(struct fb *fb)
{
    for (uint_fast8_t i = (uint_fast8_t)sizeof(fb->buf); i-- != 0;) {
        /* TODO: check what's faster */
        uint_fast8_t x = i & (uint_fast8_t)0x3u;
        uint_fast8_t y = i >> 2;

        /* zero byte (FIXME) */
        fb->buf[i] = 0;

        /* sprite render */
        for (uint_fast8_t j = (uint_fast8_t)SPRITE_COUNT; j-- != 0;) {
            /* condition(s) */
            uint_fast8_t l = y - fb->sprite[j].y;

            if (l <= (uint_fast8_t)5) {
                uint_fast8_t c1 = x - (fb->sprite[j].x >> 3);
                uint_fast8_t c2 = x - ((fb->sprite[j].x + 3) >> 3);

                uint_fast8_t rshift = (l << 2);
                uint_fast8_t px = (fb->sprite[j].data >> rshift) & 0xf0;

                /* get correct part of sprite line */
                if (c1 == 0) fb->buf[i] |= px >> (fb->sprite[j].x & 0x7);
                if (c2 == 0) fb->buf[i] |= px << (-fb->sprite[j].x & 0x7);
            }
        }
    }
}

static void max7219_refresh(struct max7219 *max7219, const struct fb *fb)
{
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT0, &fb->buf[DIGIT_COUNT * 0], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT1, &fb->buf[DIGIT_COUNT * 1], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT2, &fb->buf[DIGIT_COUNT * 2], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT3, &fb->buf[DIGIT_COUNT * 3], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT4, &fb->buf[DIGIT_COUNT * 4], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT5, &fb->buf[DIGIT_COUNT * 5], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT6, &fb->buf[DIGIT_COUNT * 6], (size_t)DIGIT_COUNT);
    (void)max7219_send_wait(max7219, MAX7219_ADDR_DIGIT7, &fb->buf[DIGIT_COUNT * 7], (size_t)DIGIT_COUNT);
}

static void display_main(void *priv)
{
    picoRTOS_assert_fatal(priv != NULL, return );

    struct board *ctx = (struct board*)priv;
    picoRTOS_tick_t ref = picoRTOS_get_tick();

    static struct fb fb;
    static struct max7219 max7219;
    (void)max7219_init(&max7219, ctx->SPI, ctx->LOAD);

    /*@+matchanyintegral@*/
    static const uint8_t zero[DIGIT_COUNT] = { 0, 0, 0, 0 };
    static const uint8_t one[DIGIT_COUNT] = { 1, 1, 1, 1 };
    static const uint8_t scan[DIGIT_COUNT] = { 7, 7, 7, 7 };
    /*@=matchanyintegral@*/

    /* turn off & configure */
    (void)max7219_send_wait(&max7219, MAX7219_ADDR_SHUTDOWN, zero, sizeof(zero));
    (void)max7219_send_wait(&max7219, MAX7219_ADDR_DECODE_MODE, zero, sizeof(zero));
    (void)max7219_send_wait(&max7219, MAX7219_ADDR_INTENSITY, one, sizeof(one));
    (void)max7219_send_wait(&max7219, MAX7219_ADDR_SCAN_LIMIT, scan, sizeof(scan));

    /* blank */
    fb_set(&fb, (uint8_t)0);
    max7219_refresh(&max7219, &fb);

    /* turn on */
    (void)max7219_send_wait(&max7219, MAX7219_ADDR_SHUTDOWN, one, sizeof(one));

begin:
    /* horizontal */
    SPRITE_SET(&fb.sprite[0], (32 + 0), 1, HEARTL);
    SPRITE_SET(&fb.sprite[1], (32 + 4), 1, HEARTR);
    SPRITE_SET(&fb.sprite[2], (32 + 8), 1, S);
    SPRITE_SET(&fb.sprite[3], (32 + 12), 1, O);
    SPRITE_SET(&fb.sprite[4], (32 + 16), 1, U);
    SPRITE_SET(&fb.sprite[5], (32 + 20), 1, N);
    SPRITE_SET(&fb.sprite[6], (32 + 24), 1, Y);
    SPRITE_SET(&fb.sprite[7], (32 + 29), 1, HEARTL);
    SPRITE_SET(&fb.sprite[8], (32 + 33), 1, HEARTR);

    while (fb.sprite[8].x != (uint_fast8_t)-4) {
        /* scroll */
        for (uint_fast8_t n = (uint_fast8_t)SPRITE_COUNT; n-- != 0;)
            fb.sprite[n].x--;

        fb_render(&fb);
        max7219_refresh(&max7219, &fb);
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(100ul));
    }

    /* vertical */
    SPRITE_SET(&fb.sprite[0], 0, -6, HEARTL);
    SPRITE_SET(&fb.sprite[1], 4, -6, HEARTR);
    SPRITE_SET(&fb.sprite[2], 8, -6, S);
    SPRITE_SET(&fb.sprite[3], 12, -6, O);
    SPRITE_SET(&fb.sprite[4], 16, -6, U);
    SPRITE_SET(&fb.sprite[5], 20, -6, N);
    SPRITE_SET(&fb.sprite[6], 24, -6, Y);
    SPRITE_SET(&fb.sprite[7], 29, -6, HEARTL);
    SPRITE_SET(&fb.sprite[8], 33, -6, HEARTR);

    while (fb.sprite[8].y != (uint_fast8_t)12) {
        /* scroll */
        for (uint_fast8_t n = (uint_fast8_t)SPRITE_COUNT; n-- != 0;)
            fb.sprite[n].y++;

        fb_render(&fb);
        max7219_refresh(&max7219, &fb);
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(100ul));
    }

    /* blink */
    SPRITE_SET(&fb.sprite[0], 0, 1, HEARTL);
    SPRITE_SET(&fb.sprite[1], 4, 1, HEARTR);
    SPRITE_SET(&fb.sprite[2], 8, 1, S);
    SPRITE_SET(&fb.sprite[3], 12, 1, O);
    SPRITE_SET(&fb.sprite[4], 16, 1, U);
    SPRITE_SET(&fb.sprite[5], 20, 1, N);
    SPRITE_SET(&fb.sprite[6], 24, 1, Y);
    SPRITE_SET(&fb.sprite[7], 29, 1, HEARTL);
    SPRITE_SET(&fb.sprite[8], 33, 1, HEARTR);

    fb_render(&fb);

    for (int i = 10; i-- != 0;) {
        fb_invert(&fb);
        max7219_refresh(&max7219, &fb);
        picoRTOS_sleep_until(&ref, PICORTOS_DELAY_MSEC(100ul));
    }

    /* The End */
    goto begin;
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
