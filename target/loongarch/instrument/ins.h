#ifndef _INS_H_
#define _INS_H_

#include "la_disasm/include/assemble.h"
#include "la_disasm/include/disasm.h"
#include "pin_types.h"

typedef struct TRANSLATION_DATA {
    void *curr_tb;

    /* ins array */
    Ins *ins_array;
    int max_ins_nr;
    int cur_ins_nr;

    /* ins list */
    int list_ins_nr;    // ins_nr in ins list
    Ins *first_ins;
    Ins *last_ins;

    TRACE trace;
} TRANSLATION_DATA;

extern TRANSLATION_DATA tr_data;
/* FIXME: maybe too large，如果插桩，给每个INS的每个call需要(1+4*args)条指令 */
#define MAX_INS_NR 5000

void tr_init(void *tb);
void tr_fini(void);

Ins *ins_alloc(uint64_t pc);
Ins *ins_copy(Ins *old);
void ins_append(Ins *ins);
void ins_remove(Ins *ins);
void ins_insert_before(Ins *old, Ins *ins);
void ins_insert_after(Ins *old, Ins *ins);

bool op_is_branch(IR2_INS_OP op);
bool op_is_direct_jmp(IR2_INS_OP op);
bool op_is_indirect_jmp(IR2_INS_OP op);
bool op_is_condition_jmp(IR2_INS_OP op);
bool op_is_load(IR2_INS_OP op);
bool op_is_store(IR2_INS_OP op);

bool opnd_is_gpr(Ins *ins, int i);

addr_t ins_target_addr(Ins *ins);

Ins *ins_b(long offset);
Ins *ins_create_0(IR2_INS_OP op);
Ins *ins_create_1(IR2_INS_OP op, int opnd0);
Ins *ins_create_2(IR2_INS_OP op, int opnd0, int opnd1);
Ins *ins_create_3(IR2_INS_OP op, int opnd0, int opnd1, int opnd2);
Ins *ins_create_4(IR2_INS_OP op, int opnd0, int opnd1, int opnd2, int opnd3);

Ins *ins_append_0(IR2_INS_OP op);
Ins *ins_append_1(IR2_INS_OP op, int opnd0);
Ins *ins_append_2(IR2_INS_OP op, int opnd0, int opnd1);
Ins *ins_append_3(IR2_INS_OP op, int opnd0, int opnd1, int opnd2);
Ins *ins_append_4(IR2_INS_OP op, int opnd0, int opnd1, int opnd2, int opnd3);

int ins_insert_before_li_d(Ins *ins, int reg, uint64_t imm);

#endif
