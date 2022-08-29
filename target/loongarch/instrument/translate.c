#include "translate.h"
#include "regs.h"
#include "error.h"
#include "bitopts.h"

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
#define TP_EXTRA_SPACE          (R21_EXTRA_SPACE + REG_LEN)

#define FCSR_EXTRA_SPACE        (TP_EXTRA_SPACE + REG_LEN)
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
    /* 2.1 GPR: save s0-sa8, fp */
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
    /* we also save ra, r21, tp */
    ins_append_3(LISA_ST_D, reg_ra, reg_sp, RA_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_x, reg_sp, R21_EXTRA_SPACE);
    ins_append_3(LISA_ST_D, reg_tp, reg_sp, TP_EXTRA_SPACE);

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

    /* 3. process args
     * a0: env
     * a1: code_cache */
    ins_append_3(LISA_OR, reg_env, reg_a0, reg_zero);
    ins_append_3(LISA_OR, reg_code_ptr, reg_a1, reg_zero);

    /* 4. save host $sp to env->host_sp */
    ins_append_3(LISA_ST_D, reg_sp, reg_env, env_offset_of_host_sp(cs));

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
    /* tr_load_registers_from_env(0xff, 0x0, 0x0, options_to_save()); */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr_is_mapped(gpr)) {
            ins_append_3(LISA_LD_D, reg_gpr_map[gpr], reg_env, env_offset_of_gpr(cs, gpr));
        }
    }

    /* 6. jump to code cache */
    /* FIXME: we can save code_ptr to SCR, then we have one more temp reg */
    ins_append_3(LISA_JIRL, reg_zero, reg_code_ptr, 0);
}

static void generate_context_switch_native_to_bt(CPUState *cs)
{
    /* 0. set return value as 0 */
    ins_append_3(LISA_OR, reg_a0, reg_zero, reg_zero);

    /* 1. store the last executed TB */
    /* FIXME: do we use this? */
    lsassert(env_offset_of_last_executed_tb(cs) >= -2048 && env_offset_of_last_executed_tb(cs) <= 2047);
    ins_append_3(LISA_ST_D, reg_code_ptr, reg_env, env_offset_of_last_executed_tb(cs));

    /* 2. store guest's next(target) pc */
    /* reg_target is setted at translating branch instruction */
    lsassert(env_offset_of_pc(cs) >= -2048 && env_offset_of_pc(cs) <= 2047);
    ins_append_3(LISA_ST_D, reg_target, reg_env, env_offset_of_pc(cs));

    /* 3. store guest registers to env */
    /* 3.1 store mapped gpr */
    /* tr_save_registers_to_env(0xff, 0x0, 0x0, options_to_save()); */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr_is_mapped(gpr)) {
            ins_append_3(LISA_ST_D, reg_gpr_map[gpr], reg_env, env_offset_of_gpr(cs, gpr));
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

    /* 4. load host $sp */
    ins_append_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(cs));

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
    /* and restore ra, r21, tp */
    ins_append_3(LISA_LD_D, reg_ra, reg_sp, RA_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_x, reg_sp, R21_EXTRA_SPACE);
    ins_append_3(LISA_LD_D, reg_tp, reg_sp, TP_EXTRA_SPACE);
    /* 6. restore sp */
    ins_append_3(LISA_ADDI_D, reg_sp, reg_sp, 512);

    /* 7. set return value */
    /* a0(return value) is set by branch instruction */
    /* ins_append_3(LISA_OR, reg_a0, reg_a0, reg_zero); */

    /* 8. return to qemu */
    ins_append_3(LISA_JIRL, reg_zero, reg_ra, 0);
}

/* bt -> native */
int la_gen_prologue(CPUState *cs, TCGContext *tcg_ctx)
{
    int ins_nr = 0;
    lsassert(context_switch_bt_to_native == 0);
    void *code_buf = tcg_ctx->code_ptr;
    context_switch_bt_to_native = (uint64_t)code_buf;

    tr_init(NULL);
    generate_context_switch_bt_to_native(cs);
    ins_nr = la_encode(tcg_ctx, (void*)context_switch_bt_to_native);
    tr_fini();

    return ins_nr;
}

/* native -> bt */
int la_gen_epilogue(CPUState *cs, TCGContext *tcg_ctx)
{
    int ins_nr = 0;
    lsassert(context_switch_native_to_bt == 0);
    void *code_buf = tcg_ctx->code_ptr;
    context_switch_native_to_bt_ret_0 = (uint64_t)code_buf;
    context_switch_native_to_bt = (uint64_t)code_buf + 4;

    tr_init(NULL);
    generate_context_switch_native_to_bt(cs);
    ins_nr = la_encode(tcg_ctx, (void*)context_switch_native_to_bt_ret_0);
    tr_fini();

    return ins_nr;
}

int INS_translate(CPUState *cs, INS pin_ins)
{
    /* 
     * Do 2 things:
     * 1. 将指令对寄存器的读写替换为对内存的读写。
     * 2. 部分指令特殊翻译：跳转指令、syscall
     * */

    /* TRANSLATION_DATA *t = &tr_data; */
    /* TranslationBlock *tb = t->curr_tb; */


    Ins *ins = ins_copy(pin_ins->origin_ins);
    ++tr_data.list_ins_nr;      // copy的ins会被加入到链表
    int ins_nr = 1;
    int before_nr = 0;
    int after_nr = 0;
    bool enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0);

    /*
     * Guest 的寄存器保存在 env->gpr[32] 中
     * 检查指令中用到的寄存器：
     * - 映射的寄存器：（上下文切换时ld/st）直接用映射的寄存器替换对应的操作数寄存器
     * - 未映射的寄存器：分配一个临时寄存器
     *   - 若指令读该寄存器：在该指令前插入一条指令，从内存load到临时寄存器
     *   - 若指令写该寄存器：在该指令后插入一条指令，将临时寄存器store到内存
     *   - 指令对寄存器的读写信息记录在 lisa_reg_access_table，若不存在，则默认
     */
    /* 1. record regs used, maping to itemp */
    /* gpr[4] 保存四个操作数用到的原寄存器(除了$zero) */
    int gpr[4] = {-1, -1, -1, -1};
    for (int i = 0; i < ins->opnd_count; i++) {
        int reg = ins->opnd[i].val;
        if (opnd_is_gpr(ins, i)) {
            gpr[i] = reg;
            /* 用映射的寄存器替换指令中原始的寄存器 */
            if (gpr_is_mapped(reg)) {
                ins->opnd[i].val = reg_alloc_gpr(reg);
            } else {
                ins->opnd[i].val = reg_map_gpr_to_itemp(reg);
            }
        }
    }

    /* 2. add LD/ST ins around origin ins */
    /* BUG: 如果两个操作数都是同一个reg，如果都被READ,岂不是LOAD两次？ */
    if (!fullregs && is_ir2_reg_access_type_valid(ins)) {
        for (int i = 0; i < 4; ++i) {
            int reg = gpr[i];
            if (reg != -1 && reg != reg_zero && !gpr_is_mapped(reg)) {
                int mapped_gpr = ins->opnd[i].val;
                if (opnd_is_gpr_read(ins, i) || opnd_is_gpr_readwrite(ins, i)) {
                    ins_insert_before(ins, ins_create_3(LISA_LD_D, mapped_gpr, reg_env, env_offset_of_gpr(cs, reg)));
                    before_nr++;
                }
                if (opnd_is_gpr_write(ins, i) || opnd_is_gpr_readwrite(ins, i)) {
                    ins_insert_after(ins, ins_create_3(LISA_ST_D, mapped_gpr, reg_env, env_offset_of_gpr(cs, reg)));
                    after_nr++;
                }
            }
        }
    } else {
#ifdef CONFIG_LMJ_DEBUG
        char msg[32];
        sprint_op(ins->op, msg);
        fprintf(stderr, "Reg access type undefined: %s\n", msg);
#endif
        for (int i = 0; i < 4; ++i) {
            int reg = gpr[i];
            if (reg != -1 && reg != reg_zero && !gpr_is_mapped(reg)) {
                /* skip duplicated regs */
                bool skip = false;
                for (int j = i - 1; j >= 0; --j) {
                    if (gpr[i] == gpr[j]) {
                        skip = true;
                        break;
                    }
                }
                if (skip) {
                    continue;
                }

                int mapped_gpr = ins->opnd[i].val;
                ins_insert_before(ins, ins_create_3(LISA_LD_D, mapped_gpr, reg_env, env_offset_of_gpr(cs, reg)));
                ins_insert_after(ins, ins_create_3(LISA_ST_D, mapped_gpr, reg_env, env_offset_of_gpr(cs, reg)));
                before_nr++;
                after_nr++;
            }
        }
    }



    /* debug: singlestep的时候为了和原生qemu对比，rdtime.d 总是返回 0 */
    if (lmj_debug && (ins->op == LISA_RDTIME_D || ins->op == LISA_RDTIMEL_W || ins->op == LISA_RDTIMEH_W)) {
        lsassert(ins->op == LISA_RDTIME_D);

        int rd = ins->opnd[0].val;
        ins_insert_before(ins, ins_create_3(LISA_OR, rd, reg_zero, reg_zero));
        before_nr++;

        ins_remove(ins);
        ins_nr--;
    }

    /* 特殊处理的指令：
     * pcaddi，直接跳转，间接跳转，syscall
     */
    /* Note: origin ins will be removed */
    if (ins->op == LISA_PCADDI || ins->op == LISA_PCADDU12I || ins->op == LISA_PCADDU18I || ins->op == LISA_PCALAU12I) {
        /* PCADD 系列指令 */
        int rd = ins->opnd[0].val;
        int li_nr = 0;
        switch (ins->op) {
            case LISA_PCADDI:
                li_nr = ins_insert_before_li_d(ins, rd, ins->pc + sign_extend(ins->opnd[1].val << 2, 22));
                before_nr += li_nr;
                break;
            case LISA_PCADDU12I:
                li_nr = ins_insert_before_li_d(ins, rd, ins->pc + sign_extend(ins->opnd[1].val << 12, 32));
                before_nr += li_nr;
                break;
            case LISA_PCADDU18I:
                li_nr = ins_insert_before_li_d(ins, rd, ins->pc + sign_extend(ins->opnd[1].val << 18, 38));
                before_nr += li_nr;
                break;
            case LISA_PCALAU12I:
                /* LISA_PCALAU12I低12位清零 */
                li_nr = ins_insert_before_li_d(ins, rd, (ins->pc + sign_extend(ins->opnd[1].val << 12, 32)) & ~0xfff);
                before_nr += li_nr;
                break;
            default:
                break;
        }

        ins_remove(ins);
        ins_nr--;
    } else if (op_is_direct_branch(ins->op)) {
        /* 直接跳转：
         * 1. 保存目标地址到 reg_target
         * 2. 跳转到上下文切换代码（推迟到重定位再做）
         * TODO: 添加stub for tb-link
         */

        /*
         * BCC的翻译：
         *   BEQ offset
         * ->
         *   BNE fallthrough
         *   reg_traget <- pc + offset
         *   B context_switch_native_to_bt
         * fallthrough:
         *   ...
         */
        tr_data.is_jmp = TRANS_NORETURN;

        int bcc_jmp_over = enable_tb_link ? 11 : 7;
        switch (ins->op) {
            case LISA_BEQZ:
                /* FIXME: maybe we can calc the magic number and patch this ins later */
                /* magic number 11: we will insert additional 10 ins before next ins */
                ins_insert_before(ins, ins_create_2(LISA_BNEZ, ins->opnd[0].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BNEZ:
                ins_insert_before(ins, ins_create_2(LISA_BEQZ, ins->opnd[0].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BCEQZ:
                ins_insert_before(ins, ins_create_2(LISA_BCNEZ, ins->opnd[0].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BCNEZ:
                ins_insert_before(ins, ins_create_2(LISA_BCEQZ, ins->opnd[0].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BEQ:
                ins_insert_before(ins, ins_create_3(LISA_BNE, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BNE:
                ins_insert_before(ins, ins_create_3(LISA_BEQ, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BLT:
                ins_insert_before(ins, ins_create_3(LISA_BGE, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BGE:
                ins_insert_before(ins, ins_create_3(LISA_BLT, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BLTU:
                ins_insert_before(ins, ins_create_3(LISA_BGEU, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                before_nr++;
                break;
            case LISA_BGEU:
                ins_insert_before(ins, ins_create_3(LISA_BLTU, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                before_nr++;
                break;
            default:
                break;
        }

        if (ins->op == LISA_BL) {
            /* save return address: $ra = PC + 4 */
            uint64_t next_pc = ins->pc + 4;
            if (gpr_is_mapped(reg_ra)) {
                before_nr += ins_insert_before_li_d(ins, reg_alloc_gpr(reg_ra), next_pc);
            } else {
                int itemp_ra = reg_alloc_itemp();
                before_nr += ins_insert_before_li_d(ins, itemp_ra, next_pc);
                ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp_ra, reg_env, env_offset_of_gpr(cs, reg_ra)));
                before_nr++;
                reg_free_itemp(itemp_ra);
            }
        }

        /* tb_link: fallthrough, this nop ins will be patched when tb_link */
        if (enable_tb_link) {
            Ins *nop = ins_create_3(LISA_OR, reg_zero, reg_zero, reg_zero);
            tr_data.jmp_ins[0] = nop;
            ins_insert_before(ins, nop);
            before_nr++;
        }

        /* set reg_target = branch target */
        before_nr += ins_insert_before_li_d(ins, reg_target, ins_target_addr(ins));

        /* set return value */
        /* if tb_link: $a0 = (tb | slot_index) */
        /* else: $a0 = 0 */
        if (enable_tb_link) {
            before_nr += ins_insert_before_li_d(ins, reg_a0, ((uint64_t)tr_data.curr_tb | 0));
        } else {
            ins_insert_before(ins, ins_create_3(LISA_ORI, reg_a0, reg_zero, reg_zero));
            before_nr++;
        }

        /* Branch to context_switch_native_to_bt, will be modify in relocation process */
        ins_insert_before(ins, ins_b(0));
        before_nr++;

        ins_remove(ins);
        ins_nr--;
    } else if (op_is_indirect_branch(ins->op)) {
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
        int li_nr = ins_insert_before_li_d(ins, reg_target, sign_extend(offset16 << 2, 18));
        ins_insert_before(ins, ins_create_3(LISA_ADD_D, reg_target, rj, reg_target));
        before_nr += li_nr + 1;

        /* GR[rd] = PC + 4 */
        /* JIRL 会写寄存器$rd，因此这里提前保存$rd */
        if (gpr[0] != -1 && gpr[0] != reg_zero) {
            uint64_t next_pc = ins->pc + 4;
            if (gpr_is_mapped(gpr[0])) {
                before_nr += ins_insert_before_li_d(ins, reg_alloc_gpr(gpr[0]), next_pc);
            } else {
                before_nr += ins_insert_before_li_d(ins, rd, next_pc);
                ins_insert_before(ins, ins_create_3(LISA_ST_D, rd, reg_env, env_offset_of_gpr(cs, gpr[0])));
                before_nr += 1;
            }
        }

        /* IBTC: 检查 tb_jmp_cache 中是否缓存了目标 tb */
        if (enable_jmp_cache) {
            int itemp_tb = reg_alloc_itemp();
            int itemp = reg_alloc_itemp();
            /* 1. get cpu->tb_jmp_cache (here offset is larger than 12bits, so load the imm) */
            li_nr = ins_insert_before_li_d(ins, itemp, env_offset_of_tb_jmp_cache(cs));
            ins_insert_before(ins, ins_create_3(LISA_ADD_D, itemp_tb, reg_env, itemp));
            /* 2. offset = hash(pc) << 3 */
            ins_insert_before(ins, ins_create_3(LISA_SRLI_D, itemp, reg_target, TB_JMP_CACHE_BITS));
            ins_insert_before(ins, ins_create_3(LISA_XOR, itemp, itemp, reg_target));
            ins_insert_before(ins, ins_create_3(LISA_ANDI, itemp, itemp, TB_JMP_CACHE_SIZE - 1));
            ins_insert_before(ins, ins_create_3(LISA_SLLI_D, itemp, itemp, 3));
            /* 3. tb = *(tb_jmp_cache + offset) */
            ins_insert_before(ins, ins_create_3(LISA_LDX_D, itemp_tb, itemp_tb, itemp));
            /* 4. if (tb != 0 && tb->pc == target) JIRL tb->tc.ptr */
            ins_insert_before(ins, ins_create_2(LISA_BEQZ, itemp_tb, 5));   /* magic num: 5 */
            ins_insert_before(ins, ins_create_3(LISA_LD_D, itemp, itemp_tb, offsetof(TranslationBlock, pc)));
            ins_insert_before(ins, ins_create_3(LISA_BNE, itemp, reg_target, 3));   /* magic num: 3 */
            ins_insert_before(ins, ins_create_3(LISA_LD_D, itemp, itemp_tb, offsetof(TranslationBlock, tc) + offsetof(struct tb_tc, ptr)));
            ins_insert_before(ins, ins_create_3(LISA_JIRL, 0, itemp, 0));
            before_nr += li_nr + 11;
            reg_free_itemp(itemp_tb);
            reg_free_itemp(itemp);
        }

        /* set return value ($a0) = 0, for not tb_link */
        ins_insert_before(ins, ins_create_3(LISA_OR, reg_a0, reg_zero, reg_zero));
        before_nr++;

        /* Branch to context_switch_native_to_bt, will be modify in redirection process */
        ins_insert_before(ins, ins_b(0));
        before_nr++;

        ins_remove(ins);
        ins_nr--;
    } else if (ins->op == LISA_SYSCALL || ins->op == LISA_BREAK) {
        tr_data.is_jmp = TRANS_NORETURN;

        int itemp = reg_alloc_itemp();

        /* 1. save exception pc */
        int li_nr = ins_insert_before_li_d(ins, itemp, ins->pc);
        ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_of_pc(cs)));
        before_nr += li_nr + 1;

        /* 2. set exception index */
        if (ins->op == LISA_SYSCALL) {
            li_nr = ins_insert_before_li_d(ins, itemp, EXCCODE_SYS);
        } else if (ins->op == LISA_BREAK){
            li_nr = ins_insert_before_li_d(ins, itemp, EXCCODE_BRK);
        }
        ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_of_exception_index(cs)));
        before_nr += li_nr + 1;


        /* 3. store guest registers to env */
        /* 3.1 store mapped gpr */
        for (int gpr = 0; gpr < 32; ++gpr) {
            if (gpr_is_mapped(gpr)) {
                ins_insert_before(ins, ins_create_3(LISA_ST_D, reg_gpr_map[gpr], reg_env, env_offset_of_gpr(cs, gpr)));
                ++before_nr;
            }
        }
        /* 3.2 store fpr[32], fcc[8], fcsr0 */
        int reg_tmp = reg_s0;
        for (int fpr = 0; fpr < 32; ++fpr) {
            ins_insert_before(ins, ins_create_3(LISA_FST_D, fpr, reg_env, env_offset_of_fpr(cs, fpr)));
            ++before_nr;
        }
        for (int fcc = 0; fcc < 8; ++fcc) {
            ins_insert_before(ins, ins_create_2(LISA_MOVCF2GR, reg_tmp, fcc));
            ins_insert_before(ins, ins_create_3(LISA_ST_B, reg_tmp, reg_env, env_offset_of_fcc(cs, fcc)));
            before_nr += 2;
        }
        ins_insert_before(ins, ins_create_2(LISA_MOVFCSR2GR, reg_tmp, reg_fcsr));
        ins_insert_before(ins, ins_create_3(LISA_ST_W, reg_tmp, reg_env, env_offset_of_fscr0(cs))); 
        before_nr += 2;

        /* 4. restore host tp */ 
        /* FIXME: just for absolutely correct, maybe no need */
        ins_insert_before(ins, ins_create_3(LISA_LD_D, reg_tmp, reg_env, env_offset_of_host_sp(cs)));
        ins_insert_before(ins, ins_create_3(LISA_LD_D, reg_tp, reg_tmp, TP_EXTRA_SPACE));
        before_nr += 2;

        /* 5. call cpu_loop_exit(CPUState *cpu) */
        /* note: arg cpu is thread local, so pass it by reg_env */
        li_nr = ins_insert_before_li_d(ins, itemp, (uint64_t)cpu_loop_exit);
        li_nr = ins_insert_before_li_d(ins, reg_a0, env_offset_of_cpu_state(cs));
        ins_insert_before(ins, ins_create_3(LISA_ADD_D, reg_a0, reg_env, reg_a0));
        /* syscall will never return */
        ins_insert_before(ins, ins_create_3(LISA_JIRL, reg_zero, itemp, 0));
        before_nr += 2 * li_nr + 2;

        reg_free_itemp(itemp);

        ins_remove(ins);
        ins_nr--;
    }

    /* free mapped itemps */
    for (int i = 0; i < 4; ++i) {
        int reg = gpr[i];
        if (reg != -1 && reg != reg_zero && !gpr_is_mapped(reg)) {
            reg_unmap_gpr_to_itemp(reg);
        }
    }
    /* debug info */
    reg_debug_itemp_all_free();

    /* 最后，确定翻译后的指令链表范围 */
    Ins *start = ins, *end = ins;
    /* TODO: a ugly fix for ins被remove的情况 */
    if (ins_nr == 0) {
        end = end->prev;
    }

    for (int i = 0; i < before_nr; ++i)
        start = start->prev;
    for (int i = 0; i < after_nr; ++i)
        end = end->next;
    ins_nr += before_nr + after_nr;
    INS_set_range(pin_ins, start, end, ins_nr);

    return ins_nr;
}

/* add an exit to BT after INS */
int INS_append_exit(INS pin_ins, uint32_t index)
{
    /* index indicate the jmp slot index */
    lsassert(index < 2);
    int num = 0;
    bool enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0);

    Ins *end;
    if (enable_tb_link) {
        /* tb_link: fallthrough, this nop ins will be patched when tb_link */
        Ins *nop = ins_create_3(LISA_OR, reg_zero, reg_zero, reg_zero);
        tr_data.jmp_ins[index] = nop;
        ins_insert_after(pin_ins->last_ins, nop);
        ++num;

        end = ins_b(0);
        ins_insert_after(nop, end);
        ++num;
    } else {
        end = ins_b(0);
        ins_insert_after(pin_ins->last_ins, end);
        ++num;
    }

    /* set reg_target = pc + 4 */
    num += ins_insert_before_li_d(end, reg_target, pin_ins->pc + 4);

    /* set return value */
    if (enable_tb_link) {
        /* tb_link: $a0 = (tb | slot_index) */
        num += ins_insert_before_li_d(end, reg_a0, ((uint64_t)tr_data.curr_tb | index));
    } else {
        /* $a0 = 0 */
        ins_insert_before(end, ins_create_3(LISA_OR, reg_a0, reg_zero, reg_zero));
        ++num;
    }

    pin_ins->last_ins = end;
    pin_ins->len += num;

    return num;
}

