#ifndef N76E003_H
#define N76E003_H

#include "gpio.h"
#include "spi.h"

struct board {
    /*@temp@*/ struct gpio *L;
    /*@temp@*/ struct gpio *LOAD;
    /*@temp@*/ struct spi *SPI;
};

int board_init(/*@out@*/ struct board *ctx);

#endif
