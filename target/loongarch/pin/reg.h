#ifndef PIN_REG_H
#define PIN_REG_H

#include <stdbool.h>

typedef enum
{
    REG_INVALID_ = 0,
    REG_NONE     = 1,
    REG_FIRST    = 2,

    // base for all kinds of registers (application, machine, pin)
    REG_RBASE,
    
    REG_GPR_BASE = REG_RBASE,
    REG_ZERO = REG_GPR_BASE,
    REG_RA,
    REG_TP,
    REG_SP,
    REG_STACK_PTR = REG_SP,
    REG_A0,
    REG_SYSCALL_ARG0 = REG_A0,
    REG_SYSCALL_RET = REG_A0,
    REG_A1,
    REG_A2,
    REG_A3,
    REG_A4,
    REG_A5,
    REG_A6,
    REG_A7,
    REG_SYSCALL_NR = REG_A7,
    REG_T0,
    REG_T1,
    REG_T2,
    REG_T3,
    REG_T4,
    REG_T5,
    REG_T6,
    REG_T7,
    REG_T8,
    REG_X,
    REG_FP,
    REG_S0,
    REG_S1,
    REG_S2,
    REG_S3,
    REG_S4,
    REG_S5,
    REG_S6,
    REG_S7,
    REG_S8,
    REG_GPR_LAST = REG_S8,

    REG_IP,
    REG_INST_PTR = REG_IP,

    REG_FPR_BASE,
    REG_F0 = REG_FPR_BASE,
    REG_F1,
    REG_F2,
    REG_F3,
    REG_F4,
    REG_F5,
    REG_F6,
    REG_F7,
    REG_F8,
    REG_F9,
    REG_F10,
    REG_F11,
    REG_F12,
    REG_F13,
    REG_F14,
    REG_F15,
    REG_F16,
    REG_F17,
    REG_F18,
    REG_F19,
    REG_F20,
    REG_F21,
    REG_F22,
    REG_F23,
    REG_F24,
    REG_F25,
    REG_F26,
    REG_F27,
    REG_F28,
    REG_F29,
    REG_F30,
    REG_F31,
    REG_FPR_LAST = REG_F31,
} REG;

#ifdef __cplusplus
extern "C" {
#endif

bool REG_is_gpr(REG REG);
bool REG_is_fpr(REG REG);
/* transform PIN's REG to internal gpr */
int REG_to_gpr(REG REG);
int REG_to_fpr(REG REG);
REG gpr_to_REG(int gpr);
REG fpr_to_REG(int fpr);

#ifdef __cplusplus
}
#endif
#endif
