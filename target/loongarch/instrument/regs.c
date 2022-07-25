#include "regs.h"
#include "bitopts.h"
#include "error.h"
#include <stdint.h>

/* --- 寄存器映射 --- */
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

 /* 返回直接映射的寄存器，返回映射的寄存器 */
int reg_alloc_gpr(int gpr) {
    lsassert(gpr_is_mapped(gpr));
    return reg_gpr_map[gpr];
}

/* --- 临时寄存器映射 --- */
static const int itemp_map[] = {
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

static const int itemp_index_map[] = {
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

#define ITEMP_NUM (sizeof(itemp_map) / sizeof(int))
static uint16_t free_itemp_mask = (1 << ITEMP_NUM) - 1;

void reg_debug_itemp_all_free(void)
{
    lsassert(free_itemp_mask == ((1 << ITEMP_NUM) - 1));
}

int reg_alloc_itemp(void)
{
    static int cur = 0;
    if (free_itemp_mask == 0) {
        lsassertm(0, "no free itemp");
    }
    while (!BitIsSet(free_itemp_mask, cur)) {
        cur = (cur + 1) % ITEMP_NUM;
    }
    free_itemp_mask = BitUnset(free_itemp_mask, cur);
    return itemp_map[cur++];
}

void reg_free_itemp(int itemp)
{
    int i = itemp_index_map[itemp];
    lsassert(!BitIsSet(free_itemp_mask, i));
    free_itemp_mask = BitSet(free_itemp_mask, i);
}



/* 记录 GPR 映射到的临时寄存器 */
static __thread int gpr_itemp_map[] = {
    [0 ... 31] = reg_invalid,
};

int reg_map_gpr_to_itemp(int gpr)
{
    if (gpr == reg_zero)
        return reg_zero;

    if (gpr_itemp_map[gpr] == reg_invalid) {
        gpr_itemp_map[gpr] = reg_alloc_itemp();
    }

    return gpr_itemp_map[gpr];
}

void reg_unmap_gpr_to_itemp(int gpr)
{
    if (gpr == reg_zero)
        return;

    int itemp = gpr_itemp_map[gpr]; 
    if (itemp != reg_invalid) {
        reg_free_itemp(itemp);
        gpr_itemp_map[gpr] = reg_invalid;
    }
    return;
}

