#ifndef PIN_REG_H
#define PIN_REG_H

typedef enum
{
    REG_INVALID_ = 0,
    REG_NONE     = 1,
    REG_FIRST    = 2,

    // base for all kinds of registers (application, machine, pin)
    REG_RBASE,
    
    REG_GR_BASE = REG_RBASE,
    REG_ZERO = REG_GR_BASE,
    REG_RA,
    REG_TP,
    REG_SP,
    REG_STACK_PTR = REG_SP,
    REG_A0,
    REG_A1,
    REG_A2,
    REG_A3,
    REG_A4,
    REG_A5,
    REG_A6,
    REG_A7,
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
    REG_GR_LAST = REG_S8,

    REG_IP,
    REG_INST_PTR = REG_IP,
} REG;

#endif
