#ifndef _LA_BITOPTS_H_
#define _LA_BITOPTS_H_

/* n: 0~63 */
#define BitSet(x, n) (x | (1 << n))
#define BitUnset(x, n) (x & ~(1 << n))
#define BitIsSet(x, n) (x & (1 << n))
#define ToggleBit(x, n) (x ^ (1 << n))

#include <stdint.h>
#include "error.h"
/* 符号扩展到64位，wide表示原始数据位宽（1~63） */
static inline uint64_t sign_extend(uint64_t addr, int wide)
{
    /* FIXME: how to deal with wide == 64? */
    lsassert(1 <= wide && wide < 64);
    /* if sign bit of addr is 1, extend */
    if (addr & (1 << (wide - 1))) {
        uint64_t ones = -1UL << wide;
        addr = ones | addr;
    } else {
        /* make high bit zero */
        addr = addr & ((1UL << wide) - 1);
    }
    return addr;
}

#endif
