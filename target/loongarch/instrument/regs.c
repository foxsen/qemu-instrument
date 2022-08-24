#include "regs.h"
#include "bitopts.h"
#include "error.h"
#include <stdint.h>

/* === 寄存器映射 === */
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
    return (reg_gpr_map[gpr] != 0);
}

 /* 返回直接映射的寄存器，返回映射的寄存器 */
int reg_alloc_gpr(int gpr) {
    lsassert(gpr_is_mapped(gpr));
    return reg_gpr_map[gpr];
}

/* === 临时寄存器映射 === */
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

static inline int get_itemp_index(int itemp)
{
    return itemp_index_map[itemp];
} 

int reg_alloc_itemp(void)
{
    static __thread int cur = 0;
    if (free_itemp_mask == 0) {
        // fprintf(stderr, "no free itemp, try to free an itemp\n");
        find_and_free_a_freeable_itemp();
        /* abort(); */
    }
    while (!BitIsSet(free_itemp_mask, cur)) {
        cur = (cur + 1) % ITEMP_NUM;
    }
    free_itemp_mask = BitUnset(free_itemp_mask, cur);
    return itemp_map[cur++];
}

void reg_free_itemp(int itemp)
{
    int idx = get_itemp_index(itemp);
    lsassert(!BitIsSet(free_itemp_mask, idx));
    free_itemp_mask = BitSet(free_itemp_mask, idx);
}




typedef enum {
    ITEMP_FREE,
    ITEMP_TMP,
    ITEMP_GPR_INUSE,
    ITEMP_GPR_FREEADBLE,
} ITEMP_STATUS;

typedef struct itemp_status {
    ITEMP_STATUS status;
    int reg;
    int dirty;
} itemp_status;

static __thread itemp_status itemp_status_table[] = {
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
    {ITEMP_FREE, reg_invalid, 0},
};

/* 记录 GPR 映射到的临时寄存器 gpr->itemp */
static __thread int gpr_itemp_map[] = {
    [0 ... 31] = reg_invalid,
};

int gpr_is_mapped_to_itemp(int gpr)
{
    lsassert(!gpr_is_mapped(gpr));
    return (gpr_itemp_map[gpr] != reg_invalid);
}

int gpr_mapped_to_itemp(int gpr)
{
    lsassert(gpr_itemp_map[gpr] != reg_invalid);
    return gpr_itemp_map[gpr];
}

int itemp_is_dirty(int itemp)
{
    return (itemp_status_table[get_itemp_index(itemp)].dirty == 1);
}

int reg_map_gpr_to_itemp(int gpr)
{
    if (gpr == reg_zero)
        return reg_zero;

    if (gpr_itemp_map[gpr] == reg_invalid) {
        gpr_itemp_map[gpr] = reg_alloc_itemp();
    }

    /* new logic */
    int itemp = gpr_itemp_map[gpr];
    int itemp_index = get_itemp_index(itemp);
    itemp_status *status = &itemp_status_table[itemp_index];
    status->status = ITEMP_GPR_INUSE;
    status->reg = gpr;

    return gpr_itemp_map[gpr];
}

void set_mapped_gpr_dirty(int gpr)
{
    int itemp = gpr_itemp_map[gpr];
    if (itemp != reg_invalid) {
        int itemp_index = get_itemp_index(itemp);
        itemp_status_table[itemp_index].dirty = 1;
    }
}

void set_mapped_gpr_freeable(int gpr)
{
    int itemp = gpr_itemp_map[gpr];
    if (itemp != reg_invalid) {
        int itemp_index = get_itemp_index(itemp);
        itemp_status_table[itemp_index].status = ITEMP_GPR_FREEADBLE;
    }
}

void reg_unmap_gpr_to_itemp(int gpr)
{
    if (gpr == reg_zero)
        return;

    int itemp = gpr_itemp_map[gpr]; 
    if (itemp != reg_invalid) {
        reg_free_itemp(itemp);

        gpr_itemp_map[gpr] = reg_invalid;

        int itemp_index = get_itemp_index(itemp);
        itemp_status_table[itemp_index].status = ITEMP_FREE;
        itemp_status_table[itemp_index].reg = reg_invalid;
        itemp_status_table[itemp_index].dirty = 0;
    }
    return;
}


int find_and_free_a_freeable_itemp(void)
{
    if (free_itemp_mask != 0)
        return -1;

    static __thread int cur = 0;
    for (int i = 0; i < ITEMP_NUM; ++i) {
        cur = (cur + 1) % ITEMP_NUM;
        if (itemp_status_table[cur].status == ITEMP_GPR_FREEADBLE && itemp_status_table[cur].dirty == 0) {
            reg_unmap_gpr_to_itemp(itemp_status_table[cur].reg);
            return -1; //itemp_map[cur];
        }
    }

    /* 如果没有FREEABLE的itemp，返回一个准备被置换的itemp */
    // fprintf(stderr, "no freeable itemp, choose a victim\n");
    cur = (cur + 1) % ITEMP_NUM;
    return itemp_map[cur];
    /* lsassertm(0, "no freeable itemp\n"); */
    /* return reg_invalid; */
}

void reg_debug_itemp_all_free(void)
{
    for (int i = 0; i < ITEMP_NUM; ++i) {
        itemp_status *status = &itemp_status_table[i];
        if (status->status == ITEMP_GPR_FREEADBLE) {
            reg_unmap_gpr_to_itemp(status->reg);
        }
        lsassert(status->status == ITEMP_FREE);
        lsassert(status->reg == reg_invalid);
        lsassert(status->dirty == 0);
    }

    lsassert(free_itemp_mask == ((1 << ITEMP_NUM) - 1));
}


#include "decoder/la_print.h"
void debug_print_itemp_status(void)
{
    fprintf(stderr, "itemp  status  reg  dirty\n");
    for (int i = 0; i < ITEMP_NUM; ++i) {
        itemp_status *status = &itemp_status_table[i];
        /* TODO: assume itemp0 == t0 */
        fprintf(stderr, "   t%d    %d    %s    %d\n", i, status->status, gpr_name(status->reg), status->dirty);
    }
}

int need_save_an_itemp_gpr(void)
{
    if (free_itemp_mask == 0) {
        // fprintf(stderr, "no free itemp, try to free an itemp\n");
        int itemp = find_and_free_a_freeable_itemp();
        if (itemp == -1)
            return -1;
        int victim_gpr = itemp_status_table[itemp_index_map[itemp]].reg;
        return victim_gpr;
    }
    return -1;
}
