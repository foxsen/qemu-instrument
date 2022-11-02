#ifndef _INS_H_
#define _INS_H_

#include "decoder/assemble.h"
#include "pin_types.h"

/* FIXME: not used */
typedef enum TransType {
    TRANS_NEXT,
    TRANS_TOO_MANY,
    TRANS_NORETURN,
    TRANS_TARGET_0,
    TRANS_TARGET_1,
} TransType;

/* context in translation process */
typedef struct TRANSLATION_DATA {
    void *curr_tb;
    TransType is_jmp;
    Ins *jmp_ins[2];    // tb两个出口的B跳转指令，会被被重定位到context_swicth

    /* ins array: 提前分配好的ins对象池，管理ins对象的分配 */
    Ins *ins_array;
    int max_ins_nr;
    int cur_ins_nr;     // 下一个待分配的ins的位置，每次开始翻译TB时，通过tr_init清零

    /* ins list: 翻译过程中生成的指令链表 */
    int list_ins_nr;    // ins_nr in ins list
    Ins *first_ins;
    Ins *last_ins;

    TRACE trace;
} TRANSLATION_DATA;

extern __thread TRANSLATION_DATA tr_data;
/* FIXME: maybe too large，如果插桩，给每个INS的每个call需要(1+4*args)条指令 */
#define MAX_INS_NR 5000

void tr_init(void *tb);
void tr_fini(void);

Ins *ins_alloc(void);
Ins *ins_copy(Ins *old);
void ins_append(Ins *ins);

/* INS系列API维护ins链表、INS、tr_data的正确性 */
/* BUG prompt: 不能保证old是属于pin_ins内的 */
void INS_append_ins(INS INS, Ins *ins);
void INS_remove_ins(INS INS, Ins *ins);
void INS_insert_ins_before(INS INS, Ins *old, Ins *ins);
void INS_insert_ins_after(INS INS, Ins *old, Ins *ins);
void INS_load_imm64_before(INS INS, Ins *ins, int reg, uint64_t imm);

Ins *ins_create_0(IR2_OPCODE op);
Ins *ins_create_1(IR2_OPCODE op, int opnd0);
Ins *ins_create_2(IR2_OPCODE op, int opnd0, int opnd1);
Ins *ins_create_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2);
Ins *ins_create_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3);

/* ins_append 系列目前只给 gen_prologue/epilogue 使用 */
Ins *ins_append_0(IR2_OPCODE op);
Ins *ins_append_1(IR2_OPCODE op, int opnd0);
Ins *ins_append_2(IR2_OPCODE op, int opnd0, int opnd1);
Ins *ins_append_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2);
Ins *ins_append_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3);

Ins *ins_nop(void);
Ins *ins_b(long offs26);
Ins *ins_pcaddi(int rd, long offs20);


/* === ins inspection === */
bool op_is_branch(IR2_OPCODE op);
bool op_is_direct_branch(IR2_OPCODE op);
bool op_is_indirect_branch(IR2_OPCODE op);
bool op_is_condition_branch(IR2_OPCODE op);
bool op_is_float_branch(IR2_OPCODE op);
bool op_is_syscall(IR2_OPCODE op);
bool op_is_ldst(IR2_OPCODE op);
bool op_is_load(IR2_OPCODE op);
bool op_is_store(IR2_OPCODE op);
bool op_is_float_load(IR2_OPCODE op);
bool op_is_float_store(IR2_OPCODE op);
bool op_is_am(IR2_OPCODE op);
bool op_is_ll(IR2_OPCODE op);
bool op_is_sc(IR2_OPCODE op);

bool opnd_is_gpr(Ins *ins, int i);
bool opnd_is_gpr_read(Ins *ins, int i);
bool opnd_is_gpr_write(Ins *ins, int i);
bool opnd_is_gpr_readwrite(Ins *ins, int i);

uint64_t ins_target_addr(Ins *ins, uint64_t pc);

#endif
