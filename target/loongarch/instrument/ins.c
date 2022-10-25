#include "ins.h"
#include "error.h"
#include <stdlib.h>
#include "regs.h"

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
        t->max_ins_nr = MAX_INS_NR;
        t->ins_array = (Ins *)calloc(t->max_ins_nr, sizeof(Ins));
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
    lsassertm(t->cur_ins_nr < t->max_ins_nr, "too many ins\n");

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
    if (old->prev != NULL) {
        old->prev->next = ins;
    }
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
    if (old->next != NULL) {
        old->next->prev = ins;
    }
    old->next = ins;

    t->list_ins_nr++;
}


bool op_is_branch(IR2_OPCODE op)
{
    return (LISA_BEQZ <= op && op <= LISA_BGEU);
}

bool op_is_direct_branch(IR2_OPCODE op)
{
    return ((LISA_BEQZ <= op && op <= LISA_BCNEZ) || (LISA_B <= op && op <= LISA_BGEU));
}

bool op_is_indirect_branch(IR2_OPCODE op)
{
    return ((op == LISA_JIRL) || (LISA_JISCR0 <= op && op <= LISA_JISCR1));
}

bool op_is_condition_branch(IR2_OPCODE op)
{
    return ((LISA_BEQZ <= op && op <= LISA_BCNEZ)
            || (LISA_BEQ <= op && op <= LISA_BGEU));
}

bool op_is_float_branch(IR2_OPCODE op)
{
    return (LISA_BCEQZ == op || LISA_BCNEZ == op);
}

bool op_is_syscall(IR2_OPCODE op)
{
    return (op == LISA_SYSCALL);
}

bool op_is_ldst(IR2_OPCODE op)
{
    return (op_is_load(op) || op_is_store(op));
}

bool op_is_load(IR2_OPCODE op)
{
    switch (op) {
    case LISA_LL_W:
    case LISA_LL_D:
    case LISA_LDPTR_W:
    case LISA_LDPTR_D:
    case LISA_LD_B:
    case LISA_LD_H:
    case LISA_LD_W:
    case LISA_LD_D:
    case LISA_LD_BU:
    case LISA_LD_HU:
    case LISA_LD_WU:
    case LISA_PRELD:
    case LISA_FLD_S:
    case LISA_FLD_D:
    case LISA_VLD:
    case LISA_XVLD:
    case LISA_LDL_W:
    case LISA_LDR_W:
    case LISA_LDL_D:
    case LISA_LDR_D:
    case LISA_VLDREPL_D:
    case LISA_VLDREPL_W:
    case LISA_VLDREPL_H:
    case LISA_VLDREPL_B:
    case LISA_XVLDREPL_D:
    case LISA_XVLDREPL_W:
    case LISA_XVLDREPL_H:
    case LISA_XVLDREPL_B:
    case LISA_LDX_B:
    case LISA_LDX_H:
    case LISA_LDX_W:
    case LISA_LDX_D:
    case LISA_LDX_BU:
    case LISA_LDX_HU:
    case LISA_LDX_WU:
    case LISA_PRELDX:
    case LISA_FLDX_S:
    case LISA_FLDX_D:
    case LISA_VLDX:
    case LISA_XVLDX:
    case LISA_FLDGT_S:
    case LISA_FLDGT_D:
    case LISA_FLDLE_S:
    case LISA_FLDLE_D:
    case LISA_LDGT_B:
    case LISA_LDGT_H:
    case LISA_LDGT_W:
    case LISA_LDGT_D:
    case LISA_LDLE_B:
    case LISA_LDLE_H:
    case LISA_LDLE_W:
    case LISA_LDLE_D:
        return true;
    default:
        return false;
    }
}

bool op_is_store(IR2_OPCODE op)
{
    switch (op) {
    case LISA_SC_W:
    case LISA_SC_D:
    case LISA_STPTR_W:
    case LISA_STPTR_D:
    case LISA_ST_B:
    case LISA_ST_H:
    case LISA_ST_W:
    case LISA_ST_D:
    case LISA_FST_S:
    case LISA_FST_D:
    case LISA_VST:
    case LISA_XVST:
    case LISA_STL_W:
    case LISA_STR_W:
    case LISA_STL_D:
    case LISA_STR_D:
    case LISA_VSTELM_D:
    case LISA_VSTELM_W:
    case LISA_VSTELM_H:
    case LISA_VSTELM_B:
    case LISA_XVSTELM_D:
    case LISA_XVSTELM_W:
    case LISA_XVSTELM_H:
    case LISA_XVSTELM_B:
    case LISA_STX_B:
    case LISA_STX_H:
    case LISA_STX_W:
    case LISA_STX_D:
    case LISA_FSTX_S:
    case LISA_FSTX_D:
    case LISA_VSTX:
    case LISA_XVSTX:
    case LISA_FSTGT_S:
    case LISA_FSTGT_D:
    case LISA_FSTLE_S:
    case LISA_FSTLE_D:
    case LISA_STGT_B:
    case LISA_STGT_H:
    case LISA_STGT_W:
    case LISA_STGT_D:
    case LISA_STLE_B:
    case LISA_STLE_H:
    case LISA_STLE_W:
    case LISA_STLE_D:
        return true;
    default:
        return false;
    }
}

bool op_is_float_load(IR2_OPCODE op)
{
    switch (op) {
    case LISA_FLD_S:
    case LISA_FLD_D:
    case LISA_VLD:
    case LISA_XVLD:
    case LISA_VLDREPL_D:
    case LISA_VLDREPL_W:
    case LISA_VLDREPL_H:
    case LISA_VLDREPL_B:
    case LISA_XVLDREPL_D:
    case LISA_XVLDREPL_W:
    case LISA_XVLDREPL_H:
    case LISA_XVLDREPL_B:
    case LISA_FLDX_S:
    case LISA_FLDX_D:
    case LISA_VLDX:
    case LISA_XVLDX:
    case LISA_FLDGT_S:
    case LISA_FLDGT_D:
    case LISA_FLDLE_S:
    case LISA_FLDLE_D:
        return true;
    default:
        return false;
    }
}

bool op_is_float_store(IR2_OPCODE op)
{
    switch (op) {
    case LISA_FST_S:
    case LISA_FST_D:
    case LISA_VST:
    case LISA_XVST:
    case LISA_VSTELM_D:
    case LISA_VSTELM_W:
    case LISA_VSTELM_H:
    case LISA_VSTELM_B:
    case LISA_XVSTELM_D:
    case LISA_XVSTELM_W:
    case LISA_XVSTELM_H:
    case LISA_XVSTELM_B:
    case LISA_FSTX_S:
    case LISA_FSTX_D:
    case LISA_VSTX:
    case LISA_XVSTX:
    case LISA_FSTGT_S:
    case LISA_FSTGT_D:
    case LISA_FSTLE_S:
    case LISA_FSTLE_D:
        return true;
    default:
        return false;
    }
}

bool op_is_am(IR2_OPCODE op)
{
    switch (op) {
    case LISA_AMSWAP_W:
    case LISA_AMSWAP_D:
    case LISA_AMADD_W:
    case LISA_AMADD_D:
    case LISA_AMAND_W:
    case LISA_AMAND_D:
    case LISA_AMOR_W:
    case LISA_AMOR_D:
    case LISA_AMXOR_W:
    case LISA_AMXOR_D:
    case LISA_AMMAX_W:
    case LISA_AMMAX_D:
    case LISA_AMMIN_W:
    case LISA_AMMIN_D:
    case LISA_AMMAX_WU:
    case LISA_AMMAX_DU:
    case LISA_AMMIN_WU:
    case LISA_AMMIN_DU:
    case LISA_AMSWAP_DB_W:
    case LISA_AMSWAP_DB_D:
    case LISA_AMADD_DB_W:
    case LISA_AMADD_DB_D:
    case LISA_AMAND_DB_W:
    case LISA_AMAND_DB_D:
    case LISA_AMOR_DB_W:
    case LISA_AMOR_DB_D:
    case LISA_AMXOR_DB_W:
    case LISA_AMXOR_DB_D:
    case LISA_AMMAX_DB_W:
    case LISA_AMMAX_DB_D:
    case LISA_AMMIN_DB_W:
    case LISA_AMMIN_DB_D:
    case LISA_AMMAX_DB_WU:
    case LISA_AMMAX_DB_DU:
    case LISA_AMMIN_DB_WU:
    case LISA_AMMIN_DB_DU:
        return true;
    default:
        return false;
    }
}

bool op_is_ll(IR2_OPCODE op)
{
    return (op == LISA_LL_W || op == LISA_LL_D);
}

bool op_is_sc(IR2_OPCODE op)
{
    return (op == LISA_SC_W || op == LISA_SC_D);
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
    lsassert(op_is_direct_branch(ins->op));
    lsassertm(ins->pc != 0x0, "pc == 0");

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
Ins *ins_nop(void)
{
    return ins_create_3(LISA_OR, reg_zero, reg_zero, reg_zero);
}

Ins *ins_b(long offs26)
{
    /* PC = PC + (offs26 << 2) */
    lsassert(offs26 == sextract64(offs26, 0, 26));
    return ins_create_1(LISA_B, offs26);
}

Ins *ins_pcaddi(int rd, long offs20)
{
    /* $rd = PC + (offs20 << 2) */
    lsassert(offs20 == sextract64(offs20, 0, 20));
    return ins_create_2(LISA_PCADDI, rd, offs20);
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
