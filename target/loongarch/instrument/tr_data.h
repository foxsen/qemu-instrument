#ifndef _TR_DATA_H_
#define _TR_DATA_H_

#include "decoder/ins.h"

typedef struct pin_ins {
    struct pin_ins *next;
    struct pin_ins *prev;
    /* origin ins info */
    uint64_t pc;
    uint32_t opcode;
    Ins *origin_ins;
    /* one origin ins is translated to an ins linked-list */
    Ins *first_ins;
    Ins *last_ins;
    int len;
    /* indicate the position where callbacks to insert */
    Ins *ibefore_next_cb;
    Ins *iafter_next_cb;
} *INS;

typedef struct pin_bbl {
    uint64_t pc;
    int nr_ins;     // INS number in BBl
    INS ins_head;
    INS ins_tail;
    struct pin_bbl *next;
    struct pin_bbl *prev;
} *BBL;

typedef struct pin_trace {
    uint64_t pc;
    int nr_bbl;
    int nr_ins;     // INS number in TRACE
    BBL bbl_head;
    BBL bbl_tail;
} *TRACE;

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

#define MAX_INS_NR (512*64)
#define MAX_PIN_INS_NR 512
#define MAX_PIN_BBL_NR 8
#define MAX_PIN_TRACE_NR 8

/* context in translation process */
typedef struct TRANSLATION_DATA {
    void *curr_tb;
    DisasType is_jmp;
    Ins *jmp_ins[2];    // tb两个出口的B跳转指令，用于tb_link

    /* ins list: 翻译过程中生成的指令链表 */
    int list_ins_nr;    // ins number in the ins list
    Ins *first_ins;
    Ins *last_ins;

    /* TB翻译得到的TRACE */
    TRACE trace;

    /* 提前分配好的ins对象池 */
    Ins *ins_array;
    int max_ins_nr;
    int cur_ins_nr; // 下一个待分配的ins的位置，每次开始翻译TB时，调用tr_init清零

    /* 提前分配好的INS/BBL/TRACE对象池 */
    int cur_INS_nr;
    int cur_BBL_nr;
    int cur_TRACE_nr;
    struct pin_ins INS_array[MAX_PIN_INS_NR];
    struct pin_bbl BBL_array[MAX_PIN_BBL_NR];
    struct pin_trace TRACE_array[MAX_PIN_TRACE_NR];
} TRANSLATION_DATA;

extern __thread TRANSLATION_DATA tr_data;

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

INS INS_alloc(uint64_t pc, uint32_t opcode, Ins *origin_ins);
BBL BBL_alloc(uint64_t pc);
TRACE TRACE_alloc(uint64_t pc);

void BBL_append_INS(BBL bbl, INS INS);
void TRACE_append_BBL(TRACE trace, BBL bbl);

/* the real type of IMG is defined in elf/symbol.cpp named struct image */
typedef void *IMG;

typedef struct pin_rtn {
    const char *name;
    uint64_t addr;
    uint64_t size;
} RTN;

#ifdef __cplusplus
extern "C" {
#endif
RTN RTN_alloc(const char *name, uint64_t addr, uint64_t size);
#ifdef __cplusplus
}
#endif

#endif

