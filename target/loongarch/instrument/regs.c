#include "regs.h"
#include "bitopts.h"
#include "error.h"
#include <stdint.h>

/* 寄存器映射 */
const int reg_gpr_map[] = {
    [reg_t0] = reg_s0,
    [reg_t1] = reg_s1,
    [reg_t2] = reg_s2,
    [reg_t3] = reg_s3,
    [reg_t4] = reg_s4,
    [reg_t5] = reg_s5,
    [reg_a0] = reg_s6,
    [reg_a1] = reg_s7,
    [reg_s0] = reg_t8,
    [31] = 0, /* make array has 32 entries */
};

#define GPR_MAPPING_NUM (sizeof(reg_gpr_map) / sizeof(int))

int gpr_is_mapped(int gpr) {
    lsassert(0 <= gpr && gpr < GPR_MAPPING_NUM);
    return (reg_gpr_map[gpr] != 0);
}

/* 直接映射的寄存器，返回映射的寄存器
 * TODO: 未映射的寄存器，分配一个临时寄存器 
 */
int reg_alloc_gpr(int gpr) {
    if (gpr_is_mapped(gpr)) {
        return reg_gpr_map[gpr];
    }
    lsassert(0);
    return 0;
}

/* 临时寄存器 */
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
    /* [reg_t8] = ITEMP8, */
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
    /* reg_zero 不用做映射 */
    if (native == reg_zero)
        return 0;

    if (native_itemp_map[native] == INVALID_TEMP) {
        native_itemp_map[native] = reg_alloc_itemp();
    }
    return native_itemp_map[native];
}

void unmap_native_reg_to_itemp(int native)
{
    if (native == reg_zero)
        return;

    int itemp = native_itemp_map[native];
    if (itemp != INVALID_TEMP) {
        reg_free_itemp(itemp);
        native_itemp_map[native] = INVALID_TEMP;
    }
}

