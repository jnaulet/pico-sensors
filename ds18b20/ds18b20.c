#include "ds18b20.h"

int ds18b20_init(struct ds18b20 *ctx, struct w1 *w1)
{
  ctx->w1 = w1;
  return 0;
}

void ds18b20_release(struct ds18b20 *ctx)
{
    /*@i@*/ (void)ctx;
}

