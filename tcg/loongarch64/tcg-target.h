/*
 * Tiny Code Generator for QEMU
 *
 * Copyright (c) 2021 WANG Xuerui <git@xen0n.name>
 *
 * Based on tcg/riscv/tcg-target.h
 *
 * Copyright (c) 2018 SiFive, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LOONGARCH_TCG_TARGET_H
#define LOONGARCH_TCG_TARGET_H

/*
 * Loongson removed the (incomplete) 32-bit support from kernel and toolchain
 * for the initial upstreaming of this architecture, so don't bother and just
 * support the LP64* ABI for now.
 */
#if defined(__loongarch64)
# define TCG_TARGET_REG_BITS 64
#else
# error unsupported LoongArch register size
#endif

#define TCG_TARGET_INSN_UNIT_SIZE 4
#define TCG_TARGET_NB_REGS 32
#define MAX_CODE_GEN_BUFFER_SIZE  SIZE_MAX

typedef enum {
    TCG_REG_ZERO,
    TCG_REG_RA,
    TCG_REG_TP,
    TCG_REG_SP,
    TCG_REG_A0,
    TCG_REG_A1,
    TCG_REG_A2,
    TCG_REG_A3,
    TCG_REG_A4,
    TCG_REG_A5,
    TCG_REG_A6,
    TCG_REG_A7,
    TCG_REG_T0,
    TCG_REG_T1,
    TCG_REG_T2,
    TCG_REG_T3,
    TCG_REG_T4,
    TCG_REG_T5,
    TCG_REG_T6,
    TCG_REG_T7,
    TCG_REG_T8,
    TCG_REG_RESERVED,
    TCG_REG_S9,
    TCG_REG_S0,
    TCG_REG_S1,
    TCG_REG_S2,
    TCG_REG_S3,
    TCG_REG_S4,
    TCG_REG_S5,
    TCG_REG_S6,
    TCG_REG_S7,
    TCG_REG_S8,

    /* aliases */
    TCG_AREG0    = TCG_REG_S0,
    TCG_REG_TMP0 = TCG_REG_T8,
    TCG_REG_TMP1 = TCG_REG_T7,
    TCG_REG_TMP2 = TCG_REG_T6,
} TCGReg;

/* used for function call generation */
#define TCG_REG_CALL_STACK              TCG_REG_SP
#define TCG_TARGET_STACK_ALIGN          16
#define TCG_TARGET_CALL_ALIGN_ARGS      1
#define TCG_TARGET_CALL_STACK_OFFSET    0

/* optional instructions */
#define TCG_TARGET_HAS_movcond_i32      0
#define TCG_TARGET_HAS_div_i32          1
#define TCG_TARGET_HAS_rem_i32          1
#define TCG_TARGET_HAS_div2_i32         0
#define TCG_TARGET_HAS_rot_i32          1
#define TCG_TARGET_HAS_deposit_i32      1
#define TCG_TARGET_HAS_extract_i32      1
#define TCG_TARGET_HAS_sextract_i32     0
#define TCG_TARGET_HAS_extract2_i32     0
#define TCG_TARGET_HAS_add2_i32         0
#define TCG_TARGET_HAS_sub2_i32         0
#define TCG_TARGET_HAS_mulu2_i32        0
#define TCG_TARGET_HAS_muls2_i32        0
#define TCG_TARGET_HAS_muluh_i32        1
#define TCG_TARGET_HAS_mulsh_i32        1
#define TCG_TARGET_HAS_ext8s_i32        1
#define TCG_TARGET_HAS_ext16s_i32       1
#define TCG_TARGET_HAS_ext8u_i32        1
#define TCG_TARGET_HAS_ext16u_i32       1
#define TCG_TARGET_HAS_bswap16_i32      1
#define TCG_TARGET_HAS_bswap32_i32      1
#define TCG_TARGET_HAS_not_i32          1
#define TCG_TARGET_HAS_neg_i32          0
#define TCG_TARGET_HAS_andc_i32         1
#define TCG_TARGET_HAS_orc_i32          1
#define TCG_TARGET_HAS_eqv_i32          0
#define TCG_TARGET_HAS_nand_i32         0
#define TCG_TARGET_HAS_nor_i32          1
#define TCG_TARGET_HAS_clz_i32          1
#define TCG_TARGET_HAS_ctz_i32          1
#define TCG_TARGET_HAS_ctpop_i32        0
#ifndef CONFIG_LMJ
#define TCG_TARGET_HAS_direct_jump      0
#else
#define TCG_TARGET_HAS_direct_jump      1
#endif
#define TCG_TARGET_HAS_brcond2          0
#define TCG_TARGET_HAS_setcond2         0
#define TCG_TARGET_HAS_qemu_st8_i32     0

/* 64-bit operations */
#define TCG_TARGET_HAS_movcond_i64      0
#define TCG_TARGET_HAS_div_i64          1
#define TCG_TARGET_HAS_rem_i64          1
#define TCG_TARGET_HAS_div2_i64         0
#define TCG_TARGET_HAS_rot_i64          1
#define TCG_TARGET_HAS_deposit_i64      1
#define TCG_TARGET_HAS_extract_i64      1
#define TCG_TARGET_HAS_sextract_i64     0
#define TCG_TARGET_HAS_extract2_i64     0
#define TCG_TARGET_HAS_extrl_i64_i32    1
#define TCG_TARGET_HAS_extrh_i64_i32    1
#define TCG_TARGET_HAS_ext8s_i64        1
#define TCG_TARGET_HAS_ext16s_i64       1
#define TCG_TARGET_HAS_ext32s_i64       1
#define TCG_TARGET_HAS_ext8u_i64        1
#define TCG_TARGET_HAS_ext16u_i64       1
#define TCG_TARGET_HAS_ext32u_i64       1
#define TCG_TARGET_HAS_bswap16_i64      1
#define TCG_TARGET_HAS_bswap32_i64      1
#define TCG_TARGET_HAS_bswap64_i64      1
#define TCG_TARGET_HAS_not_i64          1
#define TCG_TARGET_HAS_neg_i64          0
#define TCG_TARGET_HAS_andc_i64         1
#define TCG_TARGET_HAS_orc_i64          1
#define TCG_TARGET_HAS_eqv_i64          0
#define TCG_TARGET_HAS_nand_i64         0
#define TCG_TARGET_HAS_nor_i64          1
#define TCG_TARGET_HAS_clz_i64          1
#define TCG_TARGET_HAS_ctz_i64          1
#define TCG_TARGET_HAS_ctpop_i64        0
#define TCG_TARGET_HAS_add2_i64         0
#define TCG_TARGET_HAS_sub2_i64         0
#define TCG_TARGET_HAS_mulu2_i64        0
#define TCG_TARGET_HAS_muls2_i64        0
#define TCG_TARGET_HAS_muluh_i64        1
#define TCG_TARGET_HAS_mulsh_i64        1

#ifndef CONFIG_LMJ
/* not defined -- call should be eliminated at compile time */
void tb_target_set_jmp_target(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
#else

#include "qemu/bitops.h"
#include "target/loongarch/instrument/tr_data.h"
#include "target/loongarch/instrument/decoder/disasm.h"
#include "target/loongarch/instrument/decoder/assemble.h"
#include "target/loongarch/instrument/util/error.h"
/**
 * Patch the branch destination
 * @jmp_rx(and jmp_rw): address of ins to be patched
 * rx and rw are two virtual address with different prot, but mapped to the same physical address
 * @addr: target address
 */
static inline void tb_target_set_jmp_target(uintptr_t tc_ptr, uintptr_t jmp_rx,
                                            uintptr_t jmp_rw, uintptr_t addr)
{
    /* debug: bcc Out Of Range counter*/
    static uint64_t bcc16_oor = 0;
    static uint64_t bcc21_oor = 0;
    static uint64_t bcc_cnt = 0;

    /* offset is in 4 Bytes */
    int offset = (addr - jmp_rx) >> 2;
    uint32_t opcode = qatomic_read((int32_t *)jmp_rx);

    Ins ins;
    la_disasm(opcode, &ins);
    IR2_OPCODE op = ins.op;

    tr_init(NULL);  // tr_data is thread local, so init ins_array for multi-thread situation

    if (op_is_condition_branch(op)) {
        /* 条件跳转 （比较复杂）
         * tb_link后bcc的每个出口都要通过两条跳转指令到达目标TB：bcc->b(patched nop)->target_tb
         * 为了提高性能，对bcc的其中一个跳转出口(第二个出口)，直接修改bcc指令本身（另一个出口还是patch nop），从而该出口可以减少一条跳转指令
         * （虽然这样变得很复杂，但是实测会有10%的性能提升）
         * 注意：当跳转目标超出bcc的跳转范围时，退化为patch nop */
        int offset_opnd_idx;
        int offset_bits;
        /* 不同的bcc跳转范围不同 */
        if (LISA_BEQZ <= op && op <= LISA_BCNEZ) {
            offset_bits = 21;
            offset_opnd_idx = 1;
        } else if (LISA_BEQ <= op && op <= LISA_BGEU) {
            offset_bits = 16;
            offset_opnd_idx = 2;
        } else {
            lsassert(0);
        }

        if (offset == ins.opnd[offset_opnd_idx].val) {
            /* tb_reset_jump
             * we get here means BCC is not changed, so NOP ins need to be reset */
            uintptr_t nop_addr_rw = jmp_rw + (offset << 2);
            Ins *nop = ins_nop();
            opcode = la_assemble(nop);
            qatomic_set((int32_t *)nop_addr_rw, opcode);
        } else {
            /* tb_add_jump */
            ++bcc_cnt;
            if (offset == sextract64(offset, 0, offset_bits)) {
                /* patch BCC (tb_jump_unlink may also get here) */
                ins.opnd[offset_opnd_idx].val = offset;
                opcode = la_assemble(&ins);
                qatomic_set((int32_t *)jmp_rw, opcode);
            } else {
                /* offset is out of bcc branch range, patch the NOP ins */
                lsdebug("bcc%d_out_of_range: %lu/%lu, degrade to patch NOP to B\n", offset_bits, (offset_bits == 21 ? ++bcc21_oor : ++bcc16_oor), bcc_cnt);
                uintptr_t nop_addr_rx = jmp_rx + (ins.opnd[offset_opnd_idx].val << 2);
                uintptr_t nop_addr_rw = jmp_rw + (ins.opnd[offset_opnd_idx].val << 2);
                offset = (addr - nop_addr_rx) >> 2;
                Ins * b = ins_b(offset);
                opcode = la_assemble(b);
                qatomic_set((int32_t *)nop_addr_rw, opcode);
            }
        }
    } else {
        /* 直接跳转指令（B，BL） Patch nop */
        if (offset == 1) {
            /* reset to NOP */
            Ins *nop = ins_nop();
            opcode = la_assemble(nop);
            qatomic_set((int32_t *)jmp_rw, opcode);
        } else {
            /* patch NOP to B */
            Ins *b = ins_b(offset);
            opcode = la_assemble(b);
            qatomic_set((int32_t *)jmp_rw, opcode);
        }
    }
    tr_fini();
}
#endif

#define TCG_TARGET_DEFAULT_MO (0)

#define TCG_TARGET_NEED_LDST_LABELS

#define TCG_TARGET_HAS_MEMORY_BSWAP 0

#endif /* LOONGARCH_TCG_TARGET_H */
