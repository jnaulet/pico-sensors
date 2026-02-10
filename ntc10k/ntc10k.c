#include "ntc10k.h"

#define LUT_COUNT  60
#define LUT_OFFSET -30

static unsigned long lut[LUT_COUNT] = {
    162152ul, 154365ul, 146874ul, 139674ul, 132758ul, 126120ul, 119753ul, 113653ul, 107811ul, 102223ul, //
    96881ul,  91779ul,  86911ul,  82272ul,  77854ul,  73651ul,  69657ul,  65865ul,  62271ul, 58866ul,   //
    55645ul,  52602ul,  49730ul,  47023ul,  44474ul,  42079ul,  39829ul,  37719ul,  35743ul, 33895ul,   //
    32167ul,  30554ul,  29050ul,  27648ul,  26342ul,  25125ul,  23992ul,  22936ul,  21951ul, 21031ul,   //
    20168ul,  19358ul,  18594ul,  17868ul,  17177ul,  16512ul,  15867ul,  15237ul,  14615ul, 13995ul,   //
    13370ul,  12735ul,  12082ul,  11406ul,  10701ul,  9959ul,   9176ul,   8343ul,   7457ul, 6509ul
};

int ntc10k_read(unsigned long ohm, int *rtemp_c)
{
    picoRTOS_assert(ohm >= lut[0], return -EINVAL);
    picoRTOS_assert(ohm <= lut[LUT_COUNT - 1], return -EINVAL);

    for (int i = LUT_COUNT; i-- != 0;)
        if (lut[i] <= ohm) {
            *rtemp_c = i + LUT_OFFSET;
            return 0;
        }

    picoRTOS_assert_void(false);
    return -ENOENT;
}
