#include "translate.h"
#include "regs.h"
#include "util/error.h"
#include "util/bitopts.h"

/* for cpu_loop_exit */
#include "exec/exec-all.h"
extern int lmj_debug;
extern int fullregs;
extern int enable_jmp_cache;

#include "instrument.h"
#include "decoder/la_print.h"

uint64_t context_switch_bt_to_native;
uint64_t context_switch_native_to_bt_ret_0;
uint64_t context_switch_native_to_bt;

#define EXTRA_SPACE             40
#define REG_LEN                 8
#define S0_EXTRA_SPACE          EXTRA_SPACE
#define S1_EXTRA_SPACE          (S0_EXTRA_SPACE + REG_LEN)
#define S2_EXTRA_SPACE          (S1_EXTRA_SPACE + REG_LEN)
#define S3_EXTRA_SPACE          (S2_EXTRA_SPACE + REG_LEN)
#define S4_EXTRA_SPACE          (S3_EXTRA_SPACE + REG_LEN)
#define S5_EXTRA_SPACE          (S4_EXTRA_SPACE + REG_LEN)
#define S6_EXTRA_SPACE          (S5_EXTRA_SPACE + REG_LEN)
#define S7_EXTRA_SPACE          (S6_EXTRA_SPACE + REG_LEN)
#define S8_EXTRA_SPACE          (S7_EXTRA_SPACE + REG_LEN)

#define FP_EXTRA_SPACE          (S8_EXTRA_SPACE + REG_LEN)
#define RA_EXTRA_SPACE          (FP_EXTRA_SPACE + REG_LEN)
#define R21_EXTRA_SPACE         (RA_EXTRA_SPACE + REG_LEN)

#define FCSR_EXTRA_SPACE        (R21_EXTRA_SPACE + REG_LEN)
#define FCC0_EXTRA_SPACE        (FCSR_EXTRA_SPACE + REG_LEN)
#define FCC1_EXTRA_SPACE        (FCC0_EXTRA_SPACE + REG_LEN)
#define FCC2_EXTRA_SPACE        (FCC1_EXTRA_SPACE + REG_LEN)
#define FCC3_EXTRA_SPACE        (FCC2_EXTRA_SPACE + REG_LEN)
#define FCC4_EXTRA_SPACE        (FCC3_EXTRA_SPACE + REG_LEN)
#define FCC5_EXTRA_SPACE        (FCC4_EXTRA_SPACE + REG_LEN)
#define FCC6_EXTRA_SPACE        (FCC5_EXTRA_SPACE + REG_LEN)
#define FCC7_EXTRA_SPACE        (FCC6_EXTRA_SPACE + REG_LEN)
#define F24_EXTRA_SPACE         (FCC7_EXTRA_SPACE + REG_LEN)
#define F25_EXTRA_SPACE         (F24_EXTRA_SPACE + REG_LEN)
#define F26_EXTRA_SPACE         (F25_EXTRA_SPACE + REG_LEN)
#define F27_EXTRA_SPACE         (F26_EXTRA_SPACE + REG_LEN)
#define F28_EXTRA_SPACE         (F27_EXTRA_SPACE + REG_LEN)
#define F29_EXTRA_SPACE         (F28_EXTRA_SPACE + REG_LEN)
#define F30_EXTRA_SPACE         (F29_EXTRA_SPACE + REG_LEN)
#define F31_EXTRA_SPACE         (F30_EXTRA_SPACE + REG_LEN)
    
static void generate_context_switch_bt_to_native(CPUState *cs)
{
    /* 1. allocate space on the stack */
    ins_append_3(LISA_ADDI_D, reg_sp, reg_sp, -512);

    /* 2 save callee-saved LA registers. */
    /* 2.1 GPR: save s0-s8, fp */
    ins_append_3(LISA_ST_D, reg_s0, reg_sp, S0_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s1, reg_sp, S1_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s2, reg_sp, S2_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s3, reg_sp, S3_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s4, reg_sp, S4_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s5, reg_sp, S5_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s6, reg_sp, S6_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s7, reg_sp, S7_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_s8, reg_sp, S8_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_fp, reg_sp, FP_EXTRA_SPACE);
    /* we also save ra, r21 */
    ins_append_3(LISA_ST_D, reg_ra, reg_sp, RA_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_x, reg_sp, R21_EXTRA_SPACE);

    /* 2.2 FPR: save fcsr, fcc[8], fpr[24-31] */
    /* FIXME: is fcsr and fcc callee saved? */
    int reg_tmp = reg_s0;
    ins_append_2(LISA_MOVFCSR2GR, reg_tmp, reg_fcsr);
    ins_append_3(LISA_ST_W, reg_tmp, reg_sp, FCSR_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc0);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC0_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc1);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC1_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc2);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC2_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc3);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC3_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc4);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC4_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc5);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC5_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc6);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC6_EXTRA_SPACE); 
    ins_append_2(LISA_MOVCF2GR, reg_tmp, reg_fcc7);
    ins_append_3(LISA_ST_B, reg_tmp, reg_sp, FCC7_EXTRA_SPACE); 
    ins_append_3(LISA_FST_D, reg_f24, reg_sp, F24_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f25, reg_sp, F25_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f26, reg_sp, F26_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f27, reg_sp, F27_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f28, reg_sp, F28_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f29, reg_sp, F29_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f30, reg_sp, F30_EXTRA_SPACE);
    ins_append_3(LISA_FST_D, reg_f31, reg_sp, F31_EXTRA_SPACE);
    
    /* Now, all GPRs and FPRs is rewriteable (except arg a0 and a1) */

    /* 3. process args (a0 and a1)
     * a0: env
     * a1: code_cache */
    /* Move $a1 to reg_ret which is used as a temp register,
     * so its value will not be covered in the following "load mapped gpr" step */
    ins_append_3(LISA_OR, reg_env, reg_a0, reg_zero);
    ins_append_3(LISA_OR, reg_ret, reg_a1, reg_zero);

    /* 4. save host $sp and $tp to env */
    ins_append_3(LISA_ST_D, reg_sp, reg_env, env_offset_of_host_sp(cs));
    ins_append_3(LISA_ST_D, reg_tp, reg_env, env_offset_of_host_tp(cs));

    /* 5. load guest registers from env. */
    /* 5.1 load fpr[32], fcc[8], fcsr0 */
    for (int fpr = 0; fpr < 32; ++fpr) {
        ins_append_3(LISA_FLD_D, fpr, reg_env, env_offset_of_fpr(cs, fpr));
    }
    for (int fcc = 0; fcc < 8; ++fcc) {
        ins_append_3(LISA_LD_B, reg_tmp, reg_env, env_offset_of_fcc(cs, fcc)); 
        ins_append_2(LISA_MOVGR2CF, fcc, reg_tmp);
    }
    ins_append_3(LISA_LD_W, reg_tmp, reg_env, env_offset_of_fscr0(cs)); 
    ins_append_2(LISA_MOVGR2FCSR, reg_fcsr, reg_tmp);

    /* 5.2. load mapped gpr */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr != reg_zero && gpr_is_mapped(gpr)) {
            ins_append_3(LISA_LD_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(cs, gpr));
        }
    }

    /* 6. jump to code cache */
    ins_append_3(LISA_JIRL, reg_zero, reg_ret, 0);
}

static void generate_context_switch_native_to_bt(CPUState *cs)
{
    /* 1. set return value as 0 */
    ins_append_3(LISA_OR, reg_ret, reg_zero, reg_zero);

    /* 2. store guest's next(target) pc */
    /* reg_target is setted at translating branch instruction */
    lsassert(env_offset_of_pc(cs) >= -2048 && env_offset_of_pc(cs) <= 2047);
    ins_append_3(LISA_ST_D, reg_target, reg_env, env_offset_of_pc(cs));

    /* 3. store guest registers to env */
    /* 3.1 store mapped gpr */
    /* tr_save_registers_to_env(0xff, 0x0, 0x0, options_to_save()); */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr != reg_zero && gpr_is_mapped(gpr)) {
            ins_append_3(LISA_ST_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(cs, gpr));
        }
    }
    /* 3.2 store fpr[32], fcc[8], fcsr0 */
    int reg_tmp = reg_s0;
    for (int fpr = 0; fpr < 32; ++fpr) {
        ins_append_3(LISA_FST_D, fpr, reg_env, env_offset_of_fpr(cs, fpr));
    }
    for (int fcc = 0; fcc < 8; ++fcc) {
        ins_append_2(LISA_MOVCF2GR, reg_tmp, fcc);
        ins_append_3(LISA_ST_B, reg_tmp, reg_env, env_offset_of_fcc(cs, fcc)); 
    }
    ins_append_2(LISA_MOVFCSR2GR, reg_tmp, reg_fcsr);
    ins_append_3(LISA_ST_W, reg_tmp, reg_env, env_offset_of_fscr0(cs)); 

    /* 4. load host $sp, $tp */
    ins_append_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(cs));
    ins_append_3(LISA_LD_D, reg_tp, reg_env, env_offset_of_host_tp(cs));

    /* 5. restore callee-saved LA registers. */
    /* 5.1. FPR: restore fcsr, fcc[8], fpr[24-31] */
    ins_append_3(LISA_LD_W, reg_tmp, reg_sp, FCSR_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2FCSR, reg_fcsr, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC0_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc0, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC1_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc1, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC2_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc2, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC3_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc3, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC4_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc4, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC5_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc5, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC6_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc6, reg_tmp);
    ins_append_3(LISA_LD_B, reg_tmp, reg_sp, FCC7_EXTRA_SPACE); 
    ins_append_2(LISA_MOVGR2CF, reg_fcc7, reg_tmp);
    ins_append_3(LISA_FLD_D, reg_f24, reg_sp, F24_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f25, reg_sp, F25_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f26, reg_sp, F26_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f27, reg_sp, F27_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f28, reg_sp, F28_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f29, reg_sp, F29_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f30, reg_sp, F30_EXTRA_SPACE);
    ins_append_3(LISA_FLD_D, reg_f31, reg_sp, F31_EXTRA_SPACE);

    /* 5.2 GPR: restore s0-s8, fp */
    ins_append_3(LISA_LD_D, reg_s0, reg_sp, S0_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s1, reg_sp, S1_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s2, reg_sp, S2_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s3, reg_sp, S3_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s4, reg_sp, S4_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s5, reg_sp, S5_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s6, reg_sp, S6_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s7, reg_sp, S7_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_s8, reg_sp, S8_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_fp, reg_sp, FP_EXTRA_SPACE);
    /* and restore ra, r21 */
    ins_append_3(LISA_LD_D, reg_ra, reg_sp, RA_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_x, reg_sp, R21_EXTRA_SPACE);
    /* 6. restore sp */
    ins_append_3(LISA_ADDI_D, reg_sp, reg_sp, 512);

    /* 7. set return value */
    /* return value is set by branch instruction */
    ins_append_3(LISA_OR, reg_a0, reg_ret, reg_zero);

    /* 8. return to qemu */
    ins_append_3(LISA_JIRL, reg_zero, reg_ra, 0);
}

/* bt -> native */
int la_gen_prologue(CPUState *cs, TCGContext *tcg_ctx)
{
    lsassert(context_switch_bt_to_native == 0);
    void *code_buf_rw = tcg_ctx->code_ptr;
    const void *code_buf_rx = tcg_splitwx_to_rx(code_buf_rw);
    context_switch_bt_to_native = (uint64_t)code_buf_rx;

    tr_init(NULL);
    // TODO rename switch_instru_to_native
    generate_context_switch_bt_to_native(cs);
    int ins_nr = la_encode(tcg_ctx, code_buf_rw);
    lsdebug("prologue: %p, %p\n", code_buf_rw, code_buf_rw + ins_nr * 4);
    tr_fini();

    return ins_nr;
}

/* native -> bt */
int la_gen_epilogue(CPUState *cs, TCGContext *tcg_ctx)
{
    lsassert(context_switch_native_to_bt == 0);
    void *code_buf_rw = tcg_ctx->code_ptr;
    const void *code_buf_rx = tcg_splitwx_to_rx(code_buf_rw);
    context_switch_native_to_bt_ret_0 = (uint64_t)code_buf_rx;
    context_switch_native_to_bt = (uint64_t)code_buf_rx + 4;

    tr_init(NULL);
    generate_context_switch_native_to_bt(cs);
    int ins_nr = la_encode(tcg_ctx, code_buf_rw);
    lsdebug("epilogue: %p, %p\n", code_buf_rw, code_buf_rw + ins_nr * 4);
    tr_fini();

    return ins_nr;
}

/* === translation function === */
static void translate_rdtime_return_zero(CPUState *cs, INS INS, Ins *ins)
{
    /* debug: singlestep的时候为了和原生qemu对比，rdtime.d 总是返回 0 */
    lsassert(ins->op == LISA_RDTIME_D);
    int rd = ins->opnd[0].val;
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_OR, rd, reg_zero, reg_zero));
    INS_remove_ins(INS, ins);
}

static void translate_pcaddi(CPUState *cs, INS INS, Ins *ins)
{
    int rd = ins->opnd[0].val;
    switch (ins->op) {
        case LISA_PCADDI:
            INS_load_imm64_before(INS, ins, rd, INS->pc + sign_extend(ins->opnd[1].val << 2, 22));
            break;
        case LISA_PCADDU12I:
            INS_load_imm64_before(INS, ins, rd, INS->pc + sign_extend(ins->opnd[1].val << 12, 32));
            break;
        case LISA_PCADDU18I:
            INS_load_imm64_before(INS, ins, rd, INS->pc + sign_extend(ins->opnd[1].val << 18, 38));
            break;
        case LISA_PCALAU12I:
            /* LISA_PCALAU12I低12位清零 */
            INS_load_imm64_before(INS, ins, rd, (INS->pc + sign_extend(ins->opnd[1].val << 12, 32)) & ~0xfff);
            break;
        default:
            break;
    }

    INS_remove_ins(INS, ins);
}

static void translate_branch(CPUState *cs, INS INS, Ins *ins)
{
    /* 直接跳转：
     * 1. 保存目标地址到 reg_target
     * 2. 跳转到上下文切换代码（推迟到重定位再做）
     */

    /*
     * 将B/BL/BCC翻译为如下指令：
     *   1. if (BEQ) BNE fallthrough
     *   2. if (BL) $ra <- origin_ins->pc + 4
     *   3. nop
     *   4. $reg_traget <- target addr
     *   5. $reg_ret <- (tb_link) ? (tb | slot_index) : 0
     *   6. B context_switch_native_to_bt
     * fallthrough: (also the second exit for BCC)
     *   ...
     */
    bool enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0);
    tr_data.is_jmp = TRANS_NORETURN;

    /* bcc 根据条件选择出口 */
    Ins *bcc = NULL;
    int bcc_offset_opnd_idx = -1;
    if (op_is_condition_branch(ins->op)) {
        switch (ins->op) {
            case LISA_BEQZ:
                /* fallthrough offset will be set at the end of translate */
                bcc = ins_create_2(LISA_BNEZ, ins->opnd[0].val, 0);
                bcc_offset_opnd_idx = 1;
                break;
            case LISA_BNEZ:
                bcc = ins_create_2(LISA_BEQZ, ins->opnd[0].val, 0);
                bcc_offset_opnd_idx = 1;
                break;
            case LISA_BCEQZ:
                bcc = ins_create_2(LISA_BCNEZ, ins->opnd[0].val, 0);
                bcc_offset_opnd_idx = 1;
                break;
            case LISA_BCNEZ:
                bcc = ins_create_2(LISA_BCEQZ, ins->opnd[0].val, 0);
                bcc_offset_opnd_idx = 1;
                break;
            case LISA_BEQ:
                bcc = ins_create_3(LISA_BNE, ins->opnd[0].val, ins->opnd[1].val, 0);
                bcc_offset_opnd_idx = 2;
                break;
            case LISA_BNE:
                bcc = ins_create_3(LISA_BEQ, ins->opnd[0].val, ins->opnd[1].val, 0);
                bcc_offset_opnd_idx = 2;
                break;
            case LISA_BLT:
                bcc = ins_create_3(LISA_BGE, ins->opnd[0].val, ins->opnd[1].val, 0);
                bcc_offset_opnd_idx = 2;
                break;
            case LISA_BGE:
                bcc = ins_create_3(LISA_BLT, ins->opnd[0].val, ins->opnd[1].val, 0);
                bcc_offset_opnd_idx = 2;
                break;
            case LISA_BLTU:
                bcc = ins_create_3(LISA_BGEU, ins->opnd[0].val, ins->opnd[1].val, 0);
                bcc_offset_opnd_idx = 2;
                break;
            case LISA_BGEU:
                bcc = ins_create_3(LISA_BLTU, ins->opnd[0].val, ins->opnd[1].val, 0);
                bcc_offset_opnd_idx = 2;
                break;
            default:
                lsassert(0);
                break;
        }
        INS_insert_ins_before(INS, ins, bcc);

        if (enable_tb_link) {
            tr_data.jmp_ins[1] = bcc;
        }
    }

    if (ins->op == LISA_BL) {
        /* save return address: $ra = PC + 4 */
        uint64_t next_pc = INS->pc + 4;
        if (gpr_is_mapped(reg_ra)) {
            INS_load_imm64_before(INS, ins, mapped_gpr(reg_ra), next_pc);
        } else {
            int itemp_ra = reg_alloc_itemp();
            INS_load_imm64_before(INS, ins, itemp_ra, next_pc);
            INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_D, itemp_ra, reg_env, env_offset_of_gpr(cs, reg_ra)));
            reg_free_itemp(itemp_ra);
        }
    }

    /* tb_link: add a nop
     * at first exec time it will fallthrough
     * this nop ins will be patched when tb_link */
    if (enable_tb_link) {
        Ins *nop = ins_nop();
        tr_data.jmp_ins[0] = nop;
        INS_insert_ins_before(INS, ins, nop);
    }

    /* set reg_target = branch target */
    INS_load_imm64_before(INS, ins, reg_target, ins_target_addr(ins, INS->pc));

    /* set return value */
    /* ret = tb_link ? (tb | slot_index) : 0; */
    if (enable_tb_link) {
        INS_load_imm64_before(INS, ins, reg_ret, ((uint64_t)tr_data.curr_tb | 0));
    } else {
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_ORI, reg_ret, reg_zero, reg_zero));
    }

    /* Branch to context_switch_native_to_bt, will be modify in relocation process */
    Ins *b = ins_b(0);
    INS_insert_ins_before(INS, ins, b);

    if (op_is_condition_branch(ins->op)) {
        /* set bcc fallthrough offset */
        int offset = 1;
        Ins *cur = bcc;
        while (cur != b) {
            ++offset;
            cur = cur->next;
        }
        bcc->opnd[bcc_offset_opnd_idx].val = offset;
    }

    INS_remove_ins(INS, ins);
}

static void translate_jirl(CPUState *cs, INS INS, Ins *ins)
{
    /* 间接跳转：
     * 1. 计算目标地址，保存到 reg_target
     * 2. 跳转到上下文切换代码（推迟到重定位再做）
     * TODO: 添加stub for jmp_glue
     */
    tr_data.is_jmp = TRANS_NORETURN;

    int rd = ins->opnd[0].val;
    int rj = ins->opnd[1].val;
    int offset16 = ins->opnd[2].val;

    /* reg_target(PC) = GR[rj] + SignExtend(off16 << 2) */
    INS_load_imm64_before(INS, ins, reg_target, sign_extend(offset16 << 2, 18));
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_ADD_D, reg_target, rj, reg_target));

    /* GR[rd] = PC + 4 */
    /* 提前保存$rd */
    int origin_rd = INS->origin_ins->opnd[0].val;
    if (origin_rd != reg_zero) {
        uint64_t next_pc = INS->pc + 4;
        INS_load_imm64_before(INS, ins, rd, next_pc);
        if (!gpr_is_mapped(origin_rd)) {
            INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_D, rd, reg_env, env_offset_of_gpr(cs, origin_rd)));
        }
    }

    /* IBTC: 检查 tb_jmp_cache 中是否缓存了目标 tb */
    if (enable_jmp_cache) {
        int itemp_tb = reg_alloc_itemp();
        int itemp = reg_alloc_itemp();
        /* 1. get cpu->tb_jmp_cache (here offset is larger than 12bits, so load the imm) */
        INS_load_imm64_before(INS, ins, itemp, env_offset_of_tb_jmp_cache(cs));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_ADD_D, itemp_tb, reg_env, itemp));
        /* 2. hash = ((pc >> 12) ^ pc) & 0xfff (same as tb_jmp_cache_hash_func())*/
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_SRLI_D, itemp, reg_target, TB_JMP_CACHE_BITS));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_XOR, itemp, itemp, reg_target));
        // tb_jmp_cache is larger than 12 bit now
        //INS_insert_ins_before(INS, ins, ins_create_3(LISA_ANDI, itemp, itemp, TB_JMP_CACHE_SIZE - 1));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_SLLI_D, itemp, itemp, 64 - TB_JMP_CACHE_BITS));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_SRLI_D, itemp, itemp, 64 - TB_JMP_CACHE_BITS));
        /* 3. tb = *(tb_jmp_cache + (hash << 3)) */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_SLLI_D, itemp, itemp, 3));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LDX_D, itemp_tb, itemp_tb, itemp));
        /* 4. check tb valid */
        /* if (tb == 0) goto miss */
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_BEQZ, itemp_tb, 10));         /* FIXME magic num: 10 */
        /* if (tb->pc != target) goto miss */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, itemp, itemp_tb, offsetof(TranslationBlock, pc)));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_BNE, itemp, reg_target, 8));  /* FIXME magic num: 8 */
        /* jmp_cache race condition: check CF_INVALID, 9318 -> 9230(add following check) -> 8944(use LL.W) */
        /* if (jmp_lock is set) goto miss */
        lsassert(0 == (offsetof(TranslationBlock, jmp_lock) & 0b11));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LL_W, itemp, itemp_tb, offsetof(TranslationBlock, jmp_lock) >> 2));    /* FIXME why use ll_w? */
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_BNEZ, itemp, 6));             /* FIXME magic num: 6 */
        /* if (CF_INVALID != 0) goto miss */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, itemp, itemp_tb, offsetof(TranslationBlock, cflags)));
        INS_insert_ins_before(INS, ins, ins_create_4(LISA_BSTRPICK_W, itemp, itemp, 18, 18));
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_BNEZ, itemp, 3));             /* FIXME magic num: 3 */
        /* lable: jmp_cache hit */
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, itemp, itemp_tb, offsetof(TranslationBlock, tc) + offsetof(struct tb_tc, ptr)));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_JIRL, reg_zero, itemp, 0));
        reg_free_itemp(itemp_tb);
        reg_free_itemp(itemp);
    }

    /* lable: jmp_cache miss */
    /* set return value = 0, for not tb_link */
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_OR, reg_ret, reg_zero, reg_zero));

    /* Branch to context_switch_native_to_bt, will be modify in redirection process */
    INS_insert_ins_before(INS, ins, ins_b(0));

    INS_remove_ins(INS, ins);
}

static void translate_syscall(CPUState *cs, INS INS, Ins *ins)
{
    tr_data.is_jmp = TRANS_NORETURN;

    int itemp = reg_alloc_itemp();

    /* 1. save exception pc */
    INS_load_imm64_before(INS, ins, itemp, INS->pc);
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_of_pc(cs)));

    /* 2. set exception index */
    if (ins->op == LISA_SYSCALL) {
        INS_load_imm64_before(INS, ins, itemp, EXCCODE_SYS);
    } else if (ins->op == LISA_BREAK){
        INS_load_imm64_before(INS, ins, itemp, EXCCODE_BRK);
    }
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_of_exception_index(cs)));


    /* 3. store guest registers to env */
    /* 3.1 store mapped gpr */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr != reg_zero && gpr_is_mapped(gpr)) {
            INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(cs, gpr)));
        }
    }
    /* 3.2 store fpr[32], fcc[8], fcsr0 */
    int reg_tmp = reg_s0;
    for (int fpr = 0; fpr < 32; ++fpr) {
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_FST_D, fpr, reg_env, env_offset_of_fpr(cs, fpr)));
    }
    for (int fcc = 0; fcc < 8; ++fcc) {
        INS_insert_ins_before(INS, ins, ins_create_2(LISA_MOVCF2GR, reg_tmp, fcc));
        INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_B, reg_tmp, reg_env, env_offset_of_fcc(cs, fcc)));
    }
    INS_insert_ins_before(INS, ins, ins_create_2(LISA_MOVFCSR2GR, reg_tmp, reg_fcsr));
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_ST_W, reg_tmp, reg_env, env_offset_of_fscr0(cs))); 

    /* 4. restore host sp, tp */ 
    /* FIXME: just for absolutely correct, maybe no need */
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(cs)));
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, reg_tp, reg_env, env_offset_of_host_tp(cs)));

    /* 5. call cpu_loop_exit(CPUState *cpu) */
    /* note: arg cpu is thread local, so pass it by reg_env */
    INS_load_imm64_before(INS, ins, itemp, (uint64_t)cpu_loop_exit);
    INS_load_imm64_before(INS, ins, reg_a0, env_offset_of_cpu_state(cs));
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_ADD_D, reg_a0, reg_env, reg_a0));
    /* syscall will never return */
    INS_insert_ins_before(INS, ins, ins_create_3(LISA_JIRL, reg_zero, itemp, 0));

    reg_free_itemp(itemp);

    INS_remove_ins(INS, ins);
}

static void INS_reg_remapping(CPUState *cs, INS INS, Ins *ins)
{
    /*
     * Guest 的寄存器保存在 env->gpr[32] 中
     * 检查指令中用到的寄存器：
     * - 映射的寄存器：（上下文切换时已经ld/st）直接替换为映射到的物理寄存器
     * - 未映射的寄存器：分配一个临时寄存器
     *   - 若指令读该寄存器：在该指令前插入一条指令，从内存load到临时寄存器
     *   - 若指令写该寄存器：在该指令后插入一条指令，将临时寄存器store到内存
     *   - 指令对寄存器的读写信息记录在 lisa_reg_access_table，若不存在，则默认又读又写
     */

    /* 1. remapping regs and itemps */
    Ins *origin_ins = INS->origin_ins;
    for (int i = 0; i < ins->opnd_count; i++) {
        if (opnd_is_gpr(ins, i)) {
            int gpr = origin_ins->opnd[i].val;
            /* 用映射的寄存器替换指令中原始的寄存器 */
            if (gpr_is_mapped(gpr)) {
                ins->opnd[i].val = mapped_gpr(gpr);
            } else {
                ins->opnd[i].val = reg_map_gpr_to_itemp(gpr);
            }
        }
    }

    /* 2. if itemp used, add LD/ST ins around origin ins */
    /* FIXME 如果两个操作数的是同一个reg，如果都被READ，会是LOAD两次 */
    if (!fullregs && is_reg_access_type_valid(ins)) {
        for (int i = 0; i < ins->opnd_count; ++i) {
            int gpr = origin_ins->opnd[i].val;
            if (opnd_is_gpr(ins, i) && !gpr_is_mapped(gpr)) {
                int temp_gpr = ins->opnd[i].val;
                if (opnd_is_gpr_read(ins, i) || opnd_is_gpr_readwrite(ins, i)) {
                    INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, temp_gpr, reg_env, env_offset_of_gpr(cs, gpr)));
                }
                if (opnd_is_gpr_write(ins, i) || opnd_is_gpr_readwrite(ins, i)) {
                    INS_insert_ins_after(INS, ins, ins_create_3(LISA_ST_D, temp_gpr, reg_env, env_offset_of_gpr(cs, gpr)));
                }
            }
        }
    } else {

#ifdef CONFIG_LMJ_DEBUG
        char msg[32];
        sprint_op(ins->op, msg);
        lsdebug("Reg access type undefined: %s\n", msg);
#endif

        for (int i = 0; i < ins->opnd_count; ++i) {
            int gpr = origin_ins->opnd[i].val;
            if (opnd_is_gpr(ins, i) && !gpr_is_mapped(gpr)) {
                /* skip duplicated regs */
                bool skip = false;
                for (int j = i - 1; j >= 0; --j) {
                    if (origin_ins->opnd[i].val == origin_ins->opnd[j].val) {
                        skip = true;
                        break;
                    }
                }
                if (skip) {
                    continue;
                }

                int temp_gpr = ins->opnd[i].val;
                INS_insert_ins_before(INS, ins, ins_create_3(LISA_LD_D, temp_gpr, reg_env, env_offset_of_gpr(cs, gpr)));
                INS_insert_ins_after(INS, ins, ins_create_3(LISA_ST_D, temp_gpr, reg_env, env_offset_of_gpr(cs, gpr)));
            }
        }
    }
}

static void INS_free_all_itemp(CPUState *cs, INS INS)
{
    Ins *origin_ins = INS->origin_ins;
    for (int i = 0; i < origin_ins->opnd_count; ++i) {
        int reg = origin_ins->opnd[i].val;
        if (opnd_is_gpr(origin_ins, i) && !gpr_is_mapped(reg)) {
            reg_unmap_gpr_to_itemp(reg);
        }
    }
}

int INS_translate(CPUState *cs, INS INS)
{
    // FIXME 除了syscall还会有一些其他触发异常的指令，没有处理
    /* 
     * 做两件事:
     * 1. 寄存器重映射，为没有映射的寄存器分配临时寄存器
     * 2. 部分指令特殊翻译：跳转指令、syscall
     * */

    /* TRANSLATION_DATA *t = &tr_data; */
    /* TranslationBlock *tb = t->curr_tb; */
    /* bool enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0); */

    /* 1. 复制原始指令 */
    Ins *ins = ins_copy(INS->origin_ins);
    INS_append_ins(INS, ins);

    /* 2. 寄存器重映射 */
    INS_reg_remapping(cs, INS, ins);

    /* 3. 需要特殊翻译的指令：
     * pcaddi，直接跳转，间接跳转，syscall, rdtime
     */
    /* Note: origin ins may be removed */
    if (ins->op == LISA_PCADDI || ins->op == LISA_PCADDU12I || ins->op == LISA_PCADDU18I || ins->op == LISA_PCALAU12I) {
        translate_pcaddi(cs, INS, ins);
    } else if (op_is_direct_branch(ins->op)) {
        translate_branch(cs, INS, ins);
    } else if (op_is_indirect_branch(ins->op)) {
        translate_jirl(cs, INS, ins);
    } else if (ins->op == LISA_SYSCALL || ins->op == LISA_BREAK) {
        translate_syscall(cs, INS, ins);
    } else if (lmj_debug && (ins->op == LISA_RDTIME_D || ins->op == LISA_RDTIMEL_W || ins->op == LISA_RDTIMEH_W)) {
        /* debug: singlestep的时候为了和原生qemu对比，rdtime.d 总是返回 0 */
        translate_rdtime_return_zero(cs, INS, ins);
    }

    /* 4. 释放分配的临时寄存器 */
    INS_free_all_itemp(cs, INS);
    reg_debug_check_itemp_all_free();

    return INS->len;
}

void INS_append_exit(INS INS, uint32_t index)
{
    /* index indicate the jmp slot index */
    lsassert(index < 2);
    bool enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0);

    /* b context_switch_native_to_bt */
    Ins *end = ins_b(0);
    INS_append_ins(INS, end);

    /* tb_link: add a nop to be patched at tb_add_jump */
    if (enable_tb_link) {
        Ins *nop = ins_nop();
        /* for BCC's second exit, we'd like patch BCC first, rather than NOP */
        if (!op_is_condition_branch(INS->origin_ins->op)) {
            tr_data.jmp_ins[index] = nop;
        }
        INS_insert_ins_before(INS, end, nop);
    }

    /* set reg_target = pc + 4 */
    INS_load_imm64_before(INS, end, reg_target, INS->pc + 4);

    /* set return_value = tb_link ? (tb | slot_index) : 0 */
    if (enable_tb_link) {
        INS_load_imm64_before(INS, end, reg_ret, ((uint64_t)tr_data.curr_tb | index));
    } else {
        INS_insert_ins_before(INS, end, ins_create_3(LISA_ORI, reg_ret, reg_zero, 0));
    }

    INS->last_ins = end;
}

