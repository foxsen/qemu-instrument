#include "ins.h"
#include "error.h"
#include <stdlib.h>

#include "translate.h"

/* TODO: 应该放在 lsenv->tr_data */
__thread TRANSLATION_DATA tr_data;

void tr_init(void *tb)
{
    TRANSLATION_DATA *t = &tr_data;
    t->curr_tb = tb;
    t->is_jmp = TRANS_NEXT;
    t->jmp_ins[0] = NULL;
    t->jmp_ins[1] = NULL;

    /* allocate ins_array when called at the first time */
    if (t->ins_array == NULL) {
        lsassert(t->cur_ins_nr == 0);
        t->ins_array = (Ins *)calloc(MAX_INS_NR, sizeof(Ins));
        t->max_ins_nr = MAX_INS_NR;
    }
    t->cur_ins_nr = 0;

    t->list_ins_nr = 0;
    t->first_ins = NULL;
    t->last_ins = NULL;
}

void tr_fini(void)
{}


Ins *ins_alloc(uint64_t pc)
{
    TRANSLATION_DATA *t = &tr_data;
    lsassertm(t->cur_ins_nr < MAX_INS_NR, "too many ins\n");

    Ins *ins = t->ins_array + t->cur_ins_nr;
    t->cur_ins_nr++;

    ins->pc = pc;
    ins->prev = NULL;
    ins->next = NULL;
    return ins;
}

Ins *ins_copy(Ins *old)
{
    Ins *ins = ins_alloc(old->pc);
    ins->op = old->op;
    ins->opnd_count = old->opnd_count;
    ins->opnd[0].val = old->opnd[0].val;
    ins->opnd[1].val = old->opnd[1].val;
    ins->opnd[2].val = old->opnd[2].val;
    ins->opnd[3].val = old->opnd[3].val;
    return ins;
}

/* 所有的链表操作都要修改 list_ins_nr */
void ins_append(Ins *ins)
{
    TRANSLATION_DATA *t = &tr_data;
    Ins *last_ins = t->last_ins;

    if (last_ins != NULL) {
        lsassert(t->first_ins != NULL);
        last_ins->next = ins;
        ins->prev = last_ins;
        ins->next = NULL;
        t->last_ins = ins;
    } else {
        lsassert(t->first_ins == NULL);
        ins->prev = NULL;
        ins->next = NULL;
        t->first_ins= ins;
        t->last_ins = ins;
    }
    t->list_ins_nr++;
}

/* 只将元素从链表中移除，但是仍然可以通过它可以访问到前后元素 */
void ins_remove(Ins *ins)
{
    /* TODO: this is all no more use */
    TRANSLATION_DATA *t = &tr_data;
    if (t->first_ins == ins) {
        t->first_ins = ins->next;
    }
    if (t->last_ins == ins) {
        t->last_ins = ins->prev;
    }
    /* 以上这些好像可以删掉了 */

    if (ins->prev != NULL)  ins->prev->next = ins->next;
    if (ins->next != NULL)  ins->next->prev = ins->prev;

    t->list_ins_nr--;
    /* ins still have prev and last */
}

void ins_insert_before(Ins *old, Ins *ins)
{
    lsassert(old && ins);
    TRANSLATION_DATA *t = &tr_data;
    if (t->first_ins == old) {
        t->first_ins = ins;
    }
    ins->prev = old->prev;
    ins->next = old;
    if (old->prev != NULL)    old->prev->next = ins;
    old->prev = ins;

    t->list_ins_nr++;
}

void ins_insert_after(Ins *old, Ins *ins)
{
    lsassert(old && ins);
    TRANSLATION_DATA *t = &tr_data;
    if (t->last_ins == old) {
        t->last_ins = ins;
    }
    ins->prev = old;
    ins->next = old->next;
    if (old->next != NULL)    old->next->prev = ins;
    old->next = ins;

    t->list_ins_nr++;
}


bool op_is_branch(IR2_OPCODE op)
{
    return (LISA_BEQZ <= op && op <= LISA_BGEU);
}

bool op_is_direct_jmp(IR2_OPCODE op)
{
    return ((LISA_BEQZ <= op && op <= LISA_BCNEZ) || (LISA_B <= op && op <= LISA_BGEU));
}

bool op_is_indirect_jmp(IR2_OPCODE op)
{
    return (op == LISA_JIRL);
}

bool op_is_condition_jmp(IR2_OPCODE op)
{
    return ((LISA_BEQZ <= op && op <= LISA_BCNEZ)
            || (LISA_BEQ <= op && op <= LISA_BGEU));
}

bool op_is_load(IR2_OPCODE op)
{
    if (op >= LISA_LD_B && op <= LISA_LD_D) {
        return true;
    }
    if (op >= LISA_LD_BU && op <= LISA_LD_WU) {
        return true;
    }
    if (op == LISA_LL_W || op == LISA_LL_D) {
        return true;
    }
    if (op == LISA_LDPTR_W || op == LISA_LDPTR_D) {
        return true;
    }
    if (op == LISA_FLD_S || op == LISA_FLD_D) {
        return true;
    }
    if (op == LISA_VLD || op == LISA_XVLD){
        return true;
    }
    if (op >= LISA_LDL_W && op <= LISA_LDR_D){
        /* TODO: LA dont have? */
        lsassert(0);
        return true;
    }
    if (op >= LISA_VLDREPL_D && op <= LISA_VLDREPL_B){
        return true;
    }
    if (op >= LISA_XVLDREPL_D && op <= LISA_XVLDREPL_B){
        return true;
    }
    if (op == LISA_PRELD) {
        return true;
    }
    return false;
}

bool op_is_store(IR2_OPCODE op)
{
    if (op >= LISA_ST_B && op <= LISA_ST_D) {
        return true;
    }
    if (op >= LISA_STL_W && op <= LISA_STR_D) {
        return true;
    }
    if (op == LISA_SC_D || op == LISA_SC_W) {
        return true;
    }
    if (op == LISA_STPTR_W || op == LISA_STPTR_D) {
        return true;
    }
    if (op == LISA_FST_S || op == LISA_FST_D) {
        return true;
    }
    if (op == LISA_VST || op == LISA_XVST){
        return true;
    }
    return false;
}

bool opnd_is_gpr(Ins *ins, int i)
{
    IR2_OPND_TYPE type = get_ir2_opnd_type(ins, i);
    return (type == IR2_OPND_GPR);
}

bool opnd_is_gpr_read(Ins *ins, int i)
{
    lsassert(opnd_is_gpr(ins, i));
    LISA_REG_ACCESS_TYPE type = get_ir2_reg_access_type(ins, i);
    return (type == GPR_READ);
}

bool opnd_is_gpr_write(Ins *ins, int i)
{
    lsassert(opnd_is_gpr(ins, i));
    LISA_REG_ACCESS_TYPE type = get_ir2_reg_access_type(ins, i);
    return (type == GPR_WRITE);
}

bool opnd_is_gpr_readwrite(Ins *ins, int i)
{
    lsassert(opnd_is_gpr(ins, i));
    LISA_REG_ACCESS_TYPE type = get_ir2_reg_access_type(ins, i);
    return (type == GPR_READWRITE);
}

#include "bitopts.h"
uint64_t ins_target_addr(Ins *ins)
{
    lsassert(op_is_direct_jmp(ins->op));

    uint64_t pc = ins->pc;
    uint64_t offset;
    uint64_t target;
    switch (ins->op) {
        case LISA_BEQ:
        case LISA_BNE:
        case LISA_BLT:
        case LISA_BGE:
        case LISA_BLTU:
        case LISA_BGEU:
            offset = sign_extend(ins->opnd[2].val << 2, 18);
            break;
        case LISA_BEQZ:
        case LISA_BNEZ:
            offset = sign_extend(ins->opnd[1].val << 2, 23);
            break;
        case LISA_B:
        case LISA_BL:
            offset = sign_extend(ins->opnd[0].val << 2, 28);
            break;
        case LISA_BCEQZ:
        case LISA_BCNEZ:
            offset = sign_extend(ins->opnd[1].val << 2, 23);
            break;
        default:
            lsassert(0);
            offset = 0;
            break;
    }

    target = pc + offset;
    return target;
}

#include "qemu/bitops.h"
Ins *ins_b(long offset)
{
    /* note: the real offset is offset << 2 */
    lsassert(offset == sextract64(offset, 0, 26));
    return ins_create_1(LISA_B, offset);
}

Ins *ins_create_0(IR2_OPCODE op)
{
    Ins *ins = ins_alloc(0x0);
    ins->op = op;
    ins->opnd_count = 0;
    return ins;
}

Ins *ins_create_1(IR2_OPCODE op, int opnd0)
{
    Ins *ins = ins_alloc(0x0);
    ins->op = op;
    ins->opnd_count = 1;
    ins->opnd[0].val = opnd0;
    return ins;
}

Ins *ins_create_2(IR2_OPCODE op, int opnd0, int opnd1)
{
    Ins *ins = ins_alloc(0x0);
    ins->op = op;
    ins->opnd_count = 2;
    ins->opnd[0].val = opnd0;
    ins->opnd[1].val = opnd1;
    return ins;
}

Ins *ins_create_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2)
{
    Ins *ins = ins_alloc(0x0);
    ins->op = op;
    ins->opnd_count = 3;
    ins->opnd[0].val = opnd0;
    ins->opnd[1].val = opnd1;
    ins->opnd[2].val = opnd2;
    return ins;
}

Ins *ins_create_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3)
{
    Ins *ins = ins_alloc(0x0);
    ins->op = op;
    ins->opnd_count = 4;
    ins->opnd[0].val = opnd0;
    ins->opnd[1].val = opnd1;
    ins->opnd[2].val = opnd2;
    ins->opnd[3].val = opnd3;
    return ins;
}

Ins *ins_append_0(IR2_OPCODE op)
{
    Ins *ins = ins_create_0(op);
    ins_append(ins);
    return ins;
}

Ins *ins_append_1(IR2_OPCODE op, int opnd0)
{
    Ins *ins = ins_create_1(op, opnd0);
    ins_append(ins);
    return ins;
}

Ins *ins_append_2(IR2_OPCODE op, int opnd0, int opnd1)
{
    Ins *ins = ins_create_2(op, opnd0, opnd1);
    ins_append(ins);
    return ins;
}

Ins *ins_append_3(IR2_OPCODE op, int opnd0, int opnd1, int opnd2)
{
    Ins *ins = ins_create_3(op, opnd0, opnd1, opnd2);
    ins_append(ins);
    return ins;
}

Ins *ins_append_4(IR2_OPCODE op, int opnd0, int opnd1, int opnd2, int opnd3)
{
    Ins *ins = ins_create_4(op, opnd0, opnd1, opnd2, opnd3);
    ins_append(ins);
    return ins;
}


/* 在指令ins前，将64位立即数装入通用寄存器reg，返回插入的指令数 */
int ins_insert_before_li_d(Ins *ins, int reg, uint64_t imm)
{
    uint32_t lo32 = imm & 0xffffffff;
    uint32_t hi32 = imm >> 32;
    ins_insert_before(ins, ins_create_2(LISA_LU12I_W, reg, lo32 >> 12));
    ins_insert_before(ins, ins_create_3(LISA_ORI, reg, reg, lo32 & 0xfff));
    ins_insert_before(ins, ins_create_2(LISA_LU32I_D, reg, hi32 & 0xfffff));
    ins_insert_before(ins, ins_create_3(LISA_LU52I_D, reg, reg, hi32 >> 20));
    return 4;
}
