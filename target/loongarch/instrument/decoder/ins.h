#ifndef _LA_INS_H_
#define _LA_INS_H_

#include "ir2.h"

typedef IR2_OPCODE LA_OPCODE;
typedef IR2_OPND_TYPE LA_OPND_TYPE;

typedef struct {
    int val;
} LA_OPND;

typedef struct Ins {
    LA_OPCODE op;
    LA_OPND opnd[4];
    int opnd_count;

    /* linked list */
    struct Ins *prev;
    struct Ins *next;
} Ins;

#ifdef __cplusplus
extern "C" {
#endif

/* === Ins Inspection API === */
bool op_is_branch(LA_OPCODE op);
bool op_is_direct_branch(LA_OPCODE op);
bool op_is_indirect_branch(LA_OPCODE op);
bool op_is_condition_branch(LA_OPCODE op);
bool op_is_float_branch(LA_OPCODE op);
bool op_is_syscall(LA_OPCODE op);
bool op_is_ldst(LA_OPCODE op);
bool op_is_load(LA_OPCODE op);
bool op_is_store(LA_OPCODE op);
bool op_is_float_load(LA_OPCODE op);
bool op_is_float_store(LA_OPCODE op);
bool op_is_am(LA_OPCODE op);
bool op_is_ll(LA_OPCODE op);
bool op_is_sc(LA_OPCODE op);

LA_OPND_TYPE get_opnd_type(Ins *ins, int i);
LISA_REG_ACCESS_TYPE get_reg_access_type(Ins *ins, int i);
bool is_reg_access_type_valid(Ins *ins);

bool opnd_is_reg(Ins *ins, int i);
bool opnd_is_imm(Ins *ins, int i);
bool opnd_is_gpr(Ins *ins, int i);
bool opnd_is_gpr_read(Ins *ins, int i);
bool opnd_is_gpr_write(Ins *ins, int i);
bool opnd_is_gpr_readwrite(Ins *ins, int i);
bool opnd_is_read(Ins *ins, int i);
bool opnd_is_write(Ins *ins, int i);
bool opnd_is_readonly(Ins *ins, int i);
bool opnd_is_writeonly(Ins *ins, int i);
bool opnd_is_readwrite(Ins *ins, int i);

uint64_t ins_target_addr(Ins *ins, uint64_t pc);

#ifdef __cplusplus
}
#endif
#endif
