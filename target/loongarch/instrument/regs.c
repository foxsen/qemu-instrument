#include "regs.h"
#include "util/bitopts.h"
#include "util/error.h"
#include <stdint.h>

/* 映射26个寄存器的方案 */
/* === 寄存器映射 === */
/* CoreMark: 9358, 1403(no instru, instru) */
/* guest_reg -> host_reg */
static const int reg_gpr_map[] = {
    /* [0 ... 31] = reg_invalid, */
    [reg_zero] = reg_zero,
    [reg_ra] = reg_ra,
    [reg_tp] = reg_invalid,
    [reg_sp] = reg_sp,
    [reg_a0] = reg_a0,
    [reg_a1] = reg_a1,
    [reg_a2] = reg_a2,
    [reg_a3] = reg_a3,
    [reg_a4] = reg_a4,
    [reg_a5] = reg_invalid,
    [reg_a6] = reg_invalid,
    [reg_a7] = reg_invalid,
    [reg_t0] = reg_t0,
    [reg_t1] = reg_t1,
    [reg_t2] = reg_t2,
    [reg_t3] = reg_t3,
    [reg_t4] = reg_t4,
    [reg_t5] = reg_t5,
    [reg_t6] = reg_t6,
    [reg_t7] = reg_t7,
    [reg_t8] = reg_t8,
    [reg_x] = reg_invalid,
    [reg_fp] = reg_fp,
    [reg_s0] = reg_s0,
    [reg_s1] = reg_s1,
    [reg_s2] = reg_s2,
    [reg_s3] = reg_s3,
    [reg_s4] = reg_s4,
    [reg_s5] = reg_s5,
    [reg_s6] = reg_s6,
    [reg_s7] = reg_s7,
    [reg_s8] = reg_invalid,
};

static const int reg_fpr_map[] = {
    /* [0 ... 31] = reg_invalid, */
    [reg_f0] = reg_f0,
    [reg_f1] = reg_f1,
    [reg_f2] = reg_f2,
    [reg_f3] = reg_f3,
    [reg_f4] = reg_f4,
    [reg_f5] = reg_f5,
    [reg_f6] = reg_f6,
    [reg_f7] = reg_f7,
    [reg_f8] = reg_f8,
    [reg_f9] = reg_f9,
    [reg_f10] = reg_f10,
    [reg_f11] = reg_f11,
    [reg_f12] = reg_f12,
    [reg_f13] = reg_f13,
    [reg_f14] = reg_f14,
    [reg_f15] = reg_f15,
    [reg_f16] = reg_f16,
    [reg_f17] = reg_f17,
    [reg_f18] = reg_f18,
    [reg_f19] = reg_f19,
    [reg_f20] = reg_f20,
    [reg_f21] = reg_f21,
    [reg_f22] = reg_f22,
    [reg_f23] = reg_f23,
    [reg_f24] = reg_f24,
    [reg_f25] = reg_f25,
    [reg_f26] = reg_f26,
    [reg_f27] = reg_f27,
    [reg_f28] = reg_f28,
    [reg_f29] = reg_f29,
    [reg_f30] = reg_f30,
    [reg_f31] = reg_f31,
};

#define ITEMP0          0
#define ITEMP1          1
#define ITEMP2          2
#define ITEMP3          3
#define ITEMP4          4
#define ITEMP5          5
#define ITEMP6          6
#define ITEMP7          7
#define ITEMP8          8

/* === 临时寄存器映射 === */
static const int itemp_map[] = {
    [ITEMP0] = reg_a6,
    [ITEMP1] = reg_s8,
    [ITEMP2] = reg_a5,
    /* [ITEMP3] = reg_t3, */
    /* [ITEMP4] = reg_t4, */
    /* [ITEMP5] = reg_t5, */
    /* [ITEMP6] = reg_t6, */
    /* [ITEMP7] = reg_t7, */
    /* [ITEMP8] = reg_t8, */
};

static const int itemp_reverse_map[] = {
    /* [0 ... 31] = INVALID_TEMP, */
    [reg_a6] = ITEMP0,
    [reg_s8] = ITEMP1,
    [reg_a5] = ITEMP2,
    /* [reg_t3] = ITEMP3, */
    /* [reg_t4] = ITEMP4, */
    /* [reg_t5] = ITEMP5, */
    /* [reg_t6] = ITEMP6, */
    /* [reg_t7] = ITEMP7, */
    /* [reg_t8] = ITEMP8, */
};

/* 只映射9个寄存器的方案 */
/* CoreMark: 8585, 2472(no instru, instru) */
/* const int reg_gpr_map[] = { */
/*     /1* [0 ... 31] = reg_invalid, *1/ */
/*     [reg_zero] = reg_zero, */
/*     [reg_ra] = reg_invalid, */
/*     [reg_tp] = reg_invalid, */
/*     [reg_sp] = reg_invalid, */
/*     [reg_a0] = reg_s6, */
/*     [reg_a1] = reg_s7, */
/*     [reg_a2] = reg_invalid, */
/*     [reg_a3] = reg_invalid, */
/*     [reg_a4] = reg_invalid, */
/*     [reg_a5] = reg_invalid, */
/*     [reg_a6] = reg_invalid, */
/*     [reg_a7] = reg_invalid, */
/*     [reg_t0] = reg_s0, */
/*     [reg_t1] = reg_s1, */
/*     [reg_t2] = reg_s2, */
/*     [reg_t3] = reg_s3, */
/*     [reg_t4] = reg_s4, */
/*     [reg_t5] = reg_s5, */
/*     [reg_t6] = reg_invalid, */
/*     [reg_t7] = reg_invalid, */
/*     [reg_t8] = reg_invalid, */
/*     [reg_x] = reg_invalid, */
/*     [reg_fp] = reg_invalid, */
/*     [reg_s0] = reg_t8, */
/*     [reg_s1] = reg_invalid, */
/*     [reg_s2] = reg_invalid, */
/*     [reg_s3] = reg_invalid, */
/*     [reg_s4] = reg_invalid, */
/*     [reg_s5] = reg_invalid, */
/*     [reg_s6] = reg_invalid, */
/*     [reg_s7] = reg_invalid, */
/*     [reg_s8] = reg_invalid, */
/* }; */

/* static const int itemp_map[] = { */
/*     [ITEMP0] = reg_t0, */
/*     [ITEMP1] = reg_t1, */
/*     [ITEMP2] = reg_t2, */
/*     [ITEMP3] = reg_t3, */
/*     [ITEMP4] = reg_t4, */
/*     [ITEMP5] = reg_t5, */
/*     [ITEMP6] = reg_t6, */
/*     [ITEMP7] = reg_t7, */
/*     /1* [ITEMP8] = reg_t8, *1/ */
/* }; */

/* static const int itemp_index_map[] = { */
/*     /1* [0 ... 31] = INVALID_TEMP, *1/ */
/*     [reg_t0] = ITEMP0, */
/*     [reg_t1] = ITEMP1, */
/*     [reg_t2] = ITEMP2, */
/*     [reg_t3] = ITEMP3, */
/*     [reg_t4] = ITEMP4, */
/*     [reg_t5] = ITEMP5, */
/*     [reg_t6] = ITEMP6, */
/*     [reg_t7] = ITEMP7, */
/*     /1* [reg_t8] = ITEMP8, *1/ */
/* }; */

/* 兼顾插桩效率的方案（不映射caller-saved regs） */
/* CoreMark: 8783, 2364(no instru, instru) */
/* 不插桩的分数有点低，目前还没调整到最优 */
/* const int reg_gpr_map[] = { */
/*     /1* [0 ... 31] = reg_invalid, *1/ */
/*     [reg_zero] = reg_zero, */
/*     [reg_ra] = reg_ra, */
/*     [reg_tp] = reg_invalid, */
/*     [reg_sp] = reg_sp, */
/*     [reg_a0] = reg_s1, */
/*     [reg_a1] = reg_s2, */
/*     [reg_a2] = reg_invalid, */
/*     [reg_a3] = reg_invalid, */
/*     [reg_a4] = reg_invalid, */
/*     [reg_a5] = reg_invalid, */
/*     [reg_a6] = reg_invalid, */
/*     [reg_a7] = reg_invalid, */
/*     [reg_t0] = reg_s3, */
/*     [reg_t1] = reg_s4, */
/*     [reg_t2] = reg_s5, */
/*     [reg_t3] = reg_s6, */
/*     [reg_t4] = reg_s7, */
/*     [reg_t5] = reg_fp, */
/*     [reg_t6] = reg_t6, */
/*     [reg_t7] = reg_invalid, */
/*     [reg_t8] = reg_invalid, */
/*     [reg_x] = reg_invalid, */
/*     [reg_fp] = reg_invalid, */
/*     [reg_s0] = reg_s0, */
/*     [reg_s1] = reg_invalid, */
/*     [reg_s2] = reg_invalid, */
/*     [reg_s3] = reg_invalid, */
/*     [reg_s4] = reg_invalid, */
/*     [reg_s5] = reg_invalid, */
/*     [reg_s6] = reg_invalid, */
/*     [reg_s7] = reg_invalid, */
/*     [reg_s8] = reg_invalid, */
/* }; */

/* /1* === 临时寄存器映射 === *1/ */
/* static const int itemp_map[] = { */
/*     [ITEMP0] = reg_a6, */
/*     [ITEMP1] = reg_s8, */
/*     [ITEMP2] = reg_a5, */
/*     [ITEMP3] = reg_t7, */
/*     [ITEMP4] = reg_t8, */
/*     [ITEMP5] = reg_a4, */
/*     [ITEMP6] = reg_a3, */
/*     [ITEMP7] = reg_a2, */
/*     /1* [ITEMP8] = reg_t8, *1/ */
/* }; */

/* static const int itemp_index_map[] = { */
/*     /1* [0 ... 31] = INVALID_TEMP, *1/ */
/*     [reg_a6] = ITEMP0, */
/*     [reg_s8] = ITEMP1, */
/*     [reg_a5] = ITEMP2, */
/*     [reg_t7] = ITEMP3, */
/*     [reg_t8] = ITEMP4, */
/*     [reg_a4] = ITEMP5, */
/*     [reg_a3] = ITEMP6, */
/*     [reg_a2] = ITEMP7, */
/*     /1* [reg_t8] = ITEMP8, *1/ */
/* }; */


/* === 寄存器映射 === */
bool gpr_is_mapped(int gpr) {
    lsassert(0 <= gpr && gpr < 32);
    return (reg_gpr_map[gpr] != reg_invalid);
}

 /* 返回直接映射到的寄存器 */
int mapped_gpr(int gpr) {
    lsassert(gpr_is_mapped(gpr));
    return reg_gpr_map[gpr];
}

bool fpr_is_mapped(int fpr) {
    lsassert(0 <= fpr && fpr < 32);
    return (reg_fpr_map[fpr] != reg_invalid);
}

int mapped_fpr(int fpr) {
    lsassert(fpr_is_mapped(fpr));
    return reg_fpr_map[fpr];
}


/* === 临时寄存器映射 === */
static inline int get_itemp_index(int itemp)
{
    return itemp_reverse_map[itemp];
}

#define ITEMP_NUM (sizeof(itemp_map) / sizeof(int))
static __thread uint16_t free_itemp_mask = (1 << ITEMP_NUM) - 1;

int reg_alloc_itemp(void)
{
    static __thread int cur = 0;
    if (free_itemp_mask == 0) {
        lsassertm(0, "no free itemp");
        abort();
    }
    while (!BitIsSet(free_itemp_mask, cur)) {
        cur = (cur + 1) % ITEMP_NUM;
    }
    free_itemp_mask = BitUnset(free_itemp_mask, cur);
    return itemp_map[cur++];
}

void reg_free_itemp(int itemp_gpr)
{
    int idx = get_itemp_index(itemp_gpr);
    lsassert(!BitIsSet(free_itemp_mask, idx));
    free_itemp_mask = BitSet(free_itemp_mask, idx);
}

void reg_debug_check_itemp_all_free(void)
{
    lsassert(free_itemp_mask == ((1 << ITEMP_NUM) - 1));
}



/* 记录 GPR 映射到的临时寄存器 */
static __thread int gpr_itemp_map[] = {
    [0 ... 31] = reg_invalid,
};

int reg_map_gpr_to_itemp(int gpr)
{
    if (gpr == reg_zero)
        return reg_zero;

    /* 没有为gpr分配itemp则分配 */
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

