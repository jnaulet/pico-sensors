#ifndef DS18B20_H
#define DS18B20_H

#include "w1.h"
#include "picoRTOS.h"

struct ds18b20 {
  /*@temp@*/ struct w1 *w1;
};

int ds18b20_init(/*@out@*/ struct ds18b20 *ctx, struct w1 *w1);
void ds18b20_release(struct ds18b20 *ctx);

int xxx();

#endif
