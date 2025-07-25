#ifndef BOARD_H
#define BOARD_H

#include "gpio.h"
#include "w1.h"

struct board {
    /*@temp@*/ struct gpio *L;
    /*@temp@*/ struct w1 *W1;
};

int board_init(/*@out@*/ struct board *ctx);

#endif
