#ifndef _INS_H_
#define _INS_H_

#include "decoder/ins.h"
#include "pin_types.h"

/**
 * DisasType
 * DISAS_NEXT: Next instruction in program order.
 * DISAS_TOO_MANY: Too many instructions translated.
 * DISAS_NORETURN: Following code is dead.
 */
typedef enum DisasType {
    TRANS_NEXT,
    TRANS_TOO_MANY,
    TRANS_NORETURN,
} DisasType;

/* context in translation process */
typedef struct TRANSLATION_DATA {
    void *curr_tb;
    DisasType is_jmp;
    Ins *jmp_ins[2];    // tb两个出口的B跳转指令，用于tb_link

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
#define MAX_INS_NR (512*64)

void tr_init(void *tb);
void tr_fini(void);

Ins *ins_alloc(void);
Ins *ins_copy(Ins *old);
void ins_append(Ins *ins);
void ins_append_load_imm64(int reg, uint64_t imm);

/* INS API: manipulate ins in INS
 * 维护ins链表、INS、tr_data的正确性 */
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

/* ins_append API: append an ins at the end of ins_list in tr_data
 * 目前只给 gen_prologue/gen_epilogue 使用 */
Ins *ins_append_0(IR2_OPCODE op);
Ins *ins_append_1(IR2_OPCODE op, int opnd0);
Ins *ins_append_2(IR2_OPCODE op, int opnd0, int opnd1);
Ins *ins_append_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2);
Ins *ins_append_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3);

Ins *ins_nop(void);
Ins *ins_b(long offs26);
Ins *ins_pcaddi(int rd, long offs20);

#endif
