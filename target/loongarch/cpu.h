/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * QEMU LoongArch CPU
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 */

#ifndef LOONGARCH_CPU_H
#define LOONGARCH_CPU_H

#include "exec/cpu-defs.h"
#include "fpu/softfloat-types.h"
#include "hw/registerfields.h"

#define TCG_GUEST_DEFAULT_MO (0)

#define FCSR0_M1    0x1f         /* FCSR1 mask, Enables */
#define FCSR0_M2    0x1f1f0000   /* FCSR2 mask, Cause and Flags */
#define FCSR0_M3    0x300        /* FCSR3 mask, Round Mode */
#define FCSR0_RM    8            /* Round Mode bit num on fcsr0 */

FIELD(FCSR0, ENABLES, 0, 5)
FIELD(FCSR0, RM, 8, 2)
FIELD(FCSR0, FLAGS, 16, 5)
FIELD(FCSR0, CAUSE, 24, 5)

#define GET_FP_CAUSE(REG)      FIELD_EX32(REG, FCSR0, CAUSE)
#define SET_FP_CAUSE(REG, V)   FIELD_DP32(REG, FCSR0, CAUSE, V)
#define GET_FP_ENABLES(REG)    FIELD_EX32(REG, FCSR0, ENABLES)
#define SET_FP_ENABLES(REG, V) FIELD_DP32(REG, FCSR0, ENABLES, V)
#define GET_FP_FLAGS(REG)      FIELD_EX32(REG, FCSR0, FLAGS)
#define SET_FP_FLAGS(REG, V)   FIELD_DP32(REG, FCSR0, FLAGS, V)
#define UPDATE_FP_FLAGS(REG, V) \
    do { \
        (REG) |= FIELD_DP32(0, FCSR0, FLAGS, V); \
    } while (0)

#define FP_INEXACT        1
#define FP_UNDERFLOW      2
#define FP_OVERFLOW       4
#define FP_DIV0           8
#define FP_INVALID        16

#define  EXCCODE_EXTERNAL_INT   64   /* plus external interrupt number */
#define  EXCCODE_INT                 0
#define  EXCCODE_PIL                 1
#define  EXCCODE_PIS                 2
#define  EXCCODE_PIF                 3
#define  EXCCODE_PME                 4
#define  EXCCODE_PNR                 5
#define  EXCCODE_PNX                 6
#define  EXCCODE_PPI                 7
#define  EXCCODE_ADEF                8 /* Different exception subcode */
#define  EXCCODE_ADEM                8
#define  EXCCODE_ALE                 9
#define  EXCCODE_BCE                 10
#define  EXCCODE_SYS                 11
#define  EXCCODE_BRK                 12
#define  EXCCODE_INE                 13
#define  EXCCODE_IPE                 14
#define  EXCCODE_FPD                 15
#define  EXCCODE_SXD                 16
#define  EXCCODE_ASXD                17
#define  EXCCODE_FPE                 18 /* Different exception subcode */
#define  EXCCODE_VFPE                18
#define  EXCCODE_WPEF                19 /* Different exception subcode */
#define  EXCCODE_WPEM                19
#define  EXCCODE_BTD                 20
#define  EXCCODE_BTE                 21
#define  EXCCODE_DBP                 26 /* Reserved subcode used for debug */

/* cpucfg[0] bits */
FIELD(CPUCFG0, PRID, 0, 32)

/* cpucfg[1] bits */
FIELD(CPUCFG1, ARCH, 0, 2)
FIELD(CPUCFG1, PGMMU, 2, 1)
FIELD(CPUCFG1, IOCSR, 3, 1)
FIELD(CPUCFG1, PALEN, 4, 8)
FIELD(CPUCFG1, VALEN, 12, 8)
FIELD(CPUCFG1, UAL, 20, 1)
FIELD(CPUCFG1, RI, 21, 1)
FIELD(CPUCFG1, EP, 22, 1)
FIELD(CPUCFG1, RPLV, 23, 1)
FIELD(CPUCFG1, HP, 24, 1)
FIELD(CPUCFG1, IOCSR_BRD, 25, 1)
FIELD(CPUCFG1, MSG_INT, 26, 1)

/* cpucfg[2] bits */
FIELD(CPUCFG2, FP, 0, 1)
FIELD(CPUCFG2, FP_SP, 1, 1)
FIELD(CPUCFG2, FP_DP, 2, 1)
FIELD(CPUCFG2, FP_VER, 3, 3)
FIELD(CPUCFG2, LSX, 6, 1)
FIELD(CPUCFG2, LASX, 7, 1)
FIELD(CPUCFG2, COMPLEX, 8, 1)
FIELD(CPUCFG2, CRYPTO, 9, 1)
FIELD(CPUCFG2, LVZ, 10, 1)
FIELD(CPUCFG2, LVZ_VER, 11, 3)
FIELD(CPUCFG2, LLFTP, 14, 1)
FIELD(CPUCFG2, LLFTP_VER, 15, 3)
FIELD(CPUCFG2, LBT_X86, 18, 1)
FIELD(CPUCFG2, LBT_ARM, 19, 1)
FIELD(CPUCFG2, LBT_MIPS, 20, 1)
FIELD(CPUCFG2, LSPW, 21, 1)
FIELD(CPUCFG2, LAM, 22, 1)

/* cpucfg[3] bits */
FIELD(CPUCFG3, CCDMA, 0, 1)
FIELD(CPUCFG3, SFB, 1, 1)
FIELD(CPUCFG3, UCACC, 2, 1)
FIELD(CPUCFG3, LLEXC, 3, 1)
FIELD(CPUCFG3, SCDLY, 4, 1)
FIELD(CPUCFG3, LLDBAR, 5, 1)
FIELD(CPUCFG3, ITLBHMC, 6, 1)
FIELD(CPUCFG3, ICHMC, 7, 1)
FIELD(CPUCFG3, SPW_LVL, 8, 3)
FIELD(CPUCFG3, SPW_HP_HF, 11, 1)
FIELD(CPUCFG3, RVA, 12, 1)
FIELD(CPUCFG3, RVAMAX, 13, 4)

/* cpucfg[4] bits */
FIELD(CPUCFG4, CC_FREQ, 0, 32)

/* cpucfg[5] bits */
FIELD(CPUCFG5, CC_MUL, 0, 16)
FIELD(CPUCFG5, CC_DIV, 16, 16)

/* cpucfg[6] bits */
FIELD(CPUCFG6, PMP, 0, 1)
FIELD(CPUCFG6, PMVER, 1, 3)
FIELD(CPUCFG6, PMNUM, 4, 4)
FIELD(CPUCFG6, PMBITS, 8, 6)
FIELD(CPUCFG6, UPM, 14, 1)

/* cpucfg[16] bits */
FIELD(CPUCFG16, L1_IUPRE, 0, 1)
FIELD(CPUCFG16, L1_IUUNIFY, 1, 1)
FIELD(CPUCFG16, L1_DPRE, 2, 1)
FIELD(CPUCFG16, L2_IUPRE, 3, 1)
FIELD(CPUCFG16, L2_IUUNIFY, 4, 1)
FIELD(CPUCFG16, L2_IUPRIV, 5, 1)
FIELD(CPUCFG16, L2_IUINCL, 6, 1)
FIELD(CPUCFG16, L2_DPRE, 7, 1)
FIELD(CPUCFG16, L2_DPRIV, 8, 1)
FIELD(CPUCFG16, L2_DINCL, 9, 1)
FIELD(CPUCFG16, L3_IUPRE, 10, 1)
FIELD(CPUCFG16, L3_IUUNIFY, 11, 1)
FIELD(CPUCFG16, L3_IUPRIV, 12, 1)
FIELD(CPUCFG16, L3_IUINCL, 13, 1)
FIELD(CPUCFG16, L3_DPRE, 14, 1)
FIELD(CPUCFG16, L3_DPRIV, 15, 1)
FIELD(CPUCFG16, L3_DINCL, 16, 1)

/* cpucfg[17] bits */
FIELD(CPUCFG17, L1IU_WAYS, 0, 16)
FIELD(CPUCFG17, L1IU_SETS, 16, 8)
FIELD(CPUCFG17, L1IU_SIZE, 24, 7)

/* cpucfg[18] bits */
FIELD(CPUCFG18, L1D_WAYS, 0, 16)
FIELD(CPUCFG18, L1D_SETS, 16, 8)
FIELD(CPUCFG18, L1D_SIZE, 24, 7)

/* cpucfg[19] bits */
FIELD(CPUCFG19, L2IU_WAYS, 0, 16)
FIELD(CPUCFG19, L2IU_SETS, 16, 8)
FIELD(CPUCFG19, L2IU_SIZE, 24, 7)

/* cpucfg[20] bits */
FIELD(CPUCFG20, L3IU_WAYS, 0, 16)
FIELD(CPUCFG20, L3IU_SETS, 16, 8)
FIELD(CPUCFG20, L3IU_SIZE, 24, 7)

extern const char * const regnames[32];
extern const char * const fregnames[32];

typedef struct CPUArchState {
    uint64_t gpr[32];
    uint64_t pc;

    uint64_t fpr[32];
    float_status fp_status;
    bool cf[8];

    uint32_t fcsr0;
    uint32_t fcsr0_mask;

    uint32_t cpucfg[21];

    uint64_t lladdr; /* LL virtual address compared against SC */
    uint64_t llval;

    uint64_t badaddr;
} CPULoongArchState;

/**
 * LoongArchCPU:
 * @env: #CPULoongArchState
 *
 * A LoongArch CPU.
 */
struct ArchCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPULoongArchState env;
};

#define TYPE_LOONGARCH_CPU "loongarch-cpu"

OBJECT_DECLARE_CPU_TYPE(LoongArchCPU, LoongArchCPUClass,
                        LOONGARCH_CPU)

/**
 * LoongArchCPUClass:
 * @parent_realize: The parent class' realize handler.
 * @parent_reset: The parent class' reset handler.
 *
 * A LoongArch CPU model.
 */
struct LoongArchCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    DeviceReset parent_reset;
};

#define MMU_USER_IDX 3

static inline int cpu_mmu_index(CPULoongArchState *env, bool ifetch)
{
    return MMU_USER_IDX;
}

static inline void cpu_get_tb_cpu_state(CPULoongArchState *env,
                                        target_ulong *pc,
                                        target_ulong *cs_base,
                                        uint32_t *flags)
{
    *pc = env->pc;
    *cs_base = 0;
    *flags = cpu_mmu_index(env, false);
}

void loongarch_cpu_list(void);

#define cpu_list loongarch_cpu_list

#include "exec/cpu-all.h"

#define LOONGARCH_CPU_TYPE_SUFFIX "-" TYPE_LOONGARCH_CPU
#define LOONGARCH_CPU_TYPE_NAME(model) model LOONGARCH_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_LOONGARCH_CPU

#endif /* LOONGARCH_CPU_H */
