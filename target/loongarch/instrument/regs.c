#include "regs.h"
#include "bitopts.h"
#include "error.h"
#include <stdint.h>

const int reg_itemp_map[] = {
    [ITEMP0] = reg_t0,
    [ITEMP1] = reg_t1,
    [ITEMP2] = reg_t2,
    [ITEMP3] = reg_t3,
    [ITEMP4] = reg_t4,
    [ITEMP5] = reg_t5,
    [ITEMP6] = reg_t6,
    [ITEMP7] = reg_t7,
    /* [ITEMP8] = reg_t8, */
};

const int reg_itemp_reverse_map[] = {
    /* [0 ... 31] = INVALID_TEMP, */
    [reg_t0] = ITEMP0,
    [reg_t1] = ITEMP1,
    [reg_t2] = ITEMP2,
    [reg_t3] = ITEMP3,
    [reg_t4] = ITEMP4,
    [reg_t5] = ITEMP5,
    [reg_t6] = ITEMP6,
    [reg_t7] = ITEMP7,
};

#define ITEMP_NUM (sizeof(reg_itemp_map) / sizeof(int))



/* available itemp bit_map */
static uint16_t itemp_mask = (1 << ITEMP_NUM) - 1;

/* map native reg to itemp */
static int native_itemp_map[32] = {
    [0 ... 31] = INVALID_TEMP,
};

int reg_alloc_itemp(void)
{
    static int cur = 0;
    if (itemp_mask == 0) {
        lsassertm(0, "no free itemp");
    }
    while (!BitIsSet(itemp_mask, cur)) {
        cur = (cur + 1) % ITEMP_NUM;
    }
    itemp_mask = BitUnset(itemp_mask, cur);
    return reg_itemp_map[cur++];
}

void reg_free_itemp(int reg)
{
    int i = reg_itemp_reverse_map[reg];
    lsassert(!BitIsSet(itemp_mask, i));
    itemp_mask = BitSet(itemp_mask, i);
}


int map_native_reg_to_itemp(int native)
{
    if (native_itemp_map[native] == INVALID_TEMP) {
        native_itemp_map[native] = reg_alloc_itemp();
    }
    return native_itemp_map[native];
}

void unmap_native_reg_to_itemp(int native)
{
    int itemp = native_itemp_map[native];
    if (itemp != INVALID_TEMP) {
        reg_free_itemp(itemp);
        native_itemp_map[native] = INVALID_TEMP;
    }
}

