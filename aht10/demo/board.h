#ifndef BOARD_H
#define BOARD_H

#include "gpio.h"
#include "spi.h"
#include "twi.h"
#include "adc.h"
#include "uart.h"

struct board {
    /*@temp@*/ struct gpio *L;
    /*@temp@*/ struct twi *I2C;
    /*@temp@*/ struct uart *DBG;
};

int board_init(/*@out@*/ struct board *ctx);

#endif
