#include "translate.h"
#include "regs.h"
#include "error.h"
#include "bitopts.h"

/* for cpu_loop_exit */
#include "exec/exec-all.h"

int ins_translate(CPUState *cs, Ins *ins, Ins **start, Ins **end)
{
    /* 
     * Do 2 things:
     * 1. 将指令对寄存器的读写替换为对内存的读写。
     * 2. 部分指令特殊翻译：跳转指令、syscall
     * */

    /* TRANSLATION_DATA *t = &tr_data; */
    /* TranslationBlock *tb = t->curr_tb; */

    int ins_nr = 1;
    int insert_before_nr = 0;
    int insert_after_nr = 0;

    /* FIXME: 目前指令的寄存器操作数是读/写还不确定，因此一律ld + st (对于st指令，dest均为读，其中sc读后还要会写) */
    /* FIXME: 对于多个opnd使用相同的reg的情况，同一个reg会被ld/st多次 */
    /* FIXME: we can do better(延迟释放已分配的itemp) */
    /* Native reg is in the memory
     * so when:
     * - ins read gpr: load it to itemp;
     * - ins write gpr: write back to memory.
     */
    /* 1. record regs used, maping to itemp */
    /* gpr[4] 保存原寄存器 */
    int gpr[4] = {-1, -1, -1, -1};
    for (int i = 0; i < ins->opnd_count; i++) {
        if (opnd_is_gpr(ins, i)) {
            int reg = ins->opnd[i].val;
            int reg_itemp = map_native_reg_to_itemp(reg);
            gpr[i] = reg;
            /* 用临时寄存器替换指令中所使用的原始寄存器 */
            ins->opnd[i].val = reg_itemp;
        }
    }
    /* remove dup reg */
    for (int i = 3; i >= 0; --i) {
        for (int j = i - 1; j >= 0; --j) {
            if (gpr[i] == gpr[j]) {
                gpr[i] = -1;
                break;
            }
        }
    }
    /* 2. add LD/ST ins around origin ins */
    for (int i = 0; i < 4; ++i) {
        int reg = gpr[i];
        if (reg != -1) {
            int reg_itemp = map_native_reg_to_itemp(reg);
            ins_insert_before(ins, ins_create_3(LISA_LD_D, reg_itemp, reg_env, env_offset_of_gpr(cs, reg)));
            ins_insert_after(ins, ins_create_3(LISA_ST_D, reg_itemp, reg_env, env_offset_of_gpr(cs, reg)));
            insert_before_nr++;
            insert_after_nr++;
        }
    }
    /* 3. free itemps */
    for (int i = 0; i < 4; ++i) {
        if (gpr[i] != -1) {
            unmap_native_reg_to_itemp(gpr[i]);
        }
    }



    /* 特殊处理的指令：
     * rdtime, pcaddi，直接跳转，间接跳转，syscall
     */
    /* origin ins will be removed */
    if (ins->op == LISA_RDTIME_D || ins->op == LISA_RDTIMEL_W || ins->op == LISA_RDTIMEH_W) {
        lsassert(ins->op == LISA_RDTIME_D);

        /* rdtime.d 总是写 0 
         * BUG: 不写0会运行会出错
         * */
        int rd = ins->opnd[0].val;
        ins_insert_before(ins, ins_create_3(LISA_AND, rd, rd, reg_zero));
        insert_before_nr++;

        ins_remove(ins);
        ins_nr--;
    } else if (ins->op == LISA_PCADDI || ins->op == LISA_PCADDU12I || ins->op == LISA_PCADDU18I || ins->op == LISA_PCALAU12I) {
        /* PCADD 系列指令 */
        int itemp_pc = reg_alloc_itemp();
        int itemp_offset = reg_alloc_itemp();
        int li_nr = ins_insert_before_li_d(ins, itemp_pc, ins->pc);
        insert_before_nr += li_nr;
        switch (ins->op) {
            case LISA_PCADDI:
                li_nr = ins_insert_before_li_d(ins, itemp_offset, sign_extend(ins->opnd[1].val << 2, 22));
                insert_before_nr += li_nr;
                break;
            case LISA_PCADDU12I:
                li_nr = ins_insert_before_li_d(ins, itemp_offset, sign_extend(ins->opnd[1].val << 12, 32));
                insert_before_nr += li_nr;
                break;
            case LISA_PCADDU18I:
                li_nr = ins_insert_before_li_d(ins, itemp_offset, sign_extend(ins->opnd[1].val << 18, 38));
                insert_before_nr += li_nr;
                break;
            case LISA_PCALAU12I:
                li_nr = ins_insert_before_li_d(ins, itemp_offset, sign_extend(ins->opnd[1].val << 12, 32));
                insert_before_nr += li_nr;
                break;
            default:
                break;
        }

        ins_insert_before(ins, ins_create_3(LISA_ADD_D, ins->opnd[0].val, itemp_pc, itemp_offset));
        insert_before_nr++;

        /* LISA_PCALAU12I: 低12位清零 */
        if (ins->op == LISA_PCALAU12I) {
            ins_insert_before(ins, ins_create_4(LISA_BSTRINS_D, ins->opnd[0].val, reg_zero, 11, 0));
            insert_before_nr++;
        }

        reg_free_itemp(itemp_offset);
        reg_free_itemp(itemp_pc);

        ins_remove(ins);
        ins_nr--;
    } else if (op_is_direct_jmp(ins->op)) {
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
        switch (ins->op) {
            case LISA_BEQZ:
                /* magic number 7: we will insert additional 6 ins before next ins */
                ins_insert_before(ins, ins_create_2(LISA_BNEZ, ins->opnd[0].val, 7));
                insert_before_nr++;
                break;
            case LISA_BNEZ:
                ins_insert_before(ins, ins_create_2(LISA_BEQZ, ins->opnd[0].val, 7));
                insert_before_nr++;
                break;
            case LISA_BCEQZ:
                ins_insert_before(ins, ins_create_2(LISA_BCNEZ, ins->opnd[0].val, 7));
                insert_before_nr++;
                break;
            case LISA_BCNEZ:
                ins_insert_before(ins, ins_create_2(LISA_BCEQZ, ins->opnd[0].val, 7));
                insert_before_nr++;
                break;
            case LISA_BEQ:
                ins_insert_before(ins, ins_create_3(LISA_BNE, ins->opnd[0].val, ins->opnd[1].val, 7));
                insert_before_nr++;
                break;
            case LISA_BNE:
                ins_insert_before(ins, ins_create_3(LISA_BEQ, ins->opnd[0].val, ins->opnd[1].val, 7));
                insert_before_nr++;
                break;
            case LISA_BLT:
                ins_insert_before(ins, ins_create_3(LISA_BGE, ins->opnd[0].val, ins->opnd[1].val, 7));
                insert_before_nr++;
                break;
            case LISA_BGE:
                ins_insert_before(ins, ins_create_3(LISA_BLT, ins->opnd[0].val, ins->opnd[1].val, 7));
                insert_before_nr++;
                break;
            case LISA_BLTU:
                ins_insert_before(ins, ins_create_3(LISA_BGEU, ins->opnd[0].val, ins->opnd[1].val, 7));
                insert_before_nr++;
                break;
            case LISA_BGEU:
                ins_insert_before(ins, ins_create_3(LISA_BLTU, ins->opnd[0].val, ins->opnd[1].val, 7));
                insert_before_nr++;
                break;
            default:
                break;
        }

        if (ins->op == LISA_BL) {
            /* save return address: GR[1] = PC + 4 */
            uint64_t next_pc = ins->pc + 4;
            int itemp_ra = reg_alloc_itemp();
            int li_nr = ins_insert_before_li_d(ins, itemp_ra, next_pc);
            ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp_ra, reg_env, env_offset_of_gpr(cs, reg_ra)));
            insert_before_nr += li_nr + 1;
            reg_free_itemp(itemp_ra);
        }


        /* set reg_tb = tb_addr, for tb link */
        /* TranslationBlock *tb = tr_data.curr_tb; */
        /* uint64_t tb_addr = (uint64_t)tb; */
        /* /1* todo: load tb_addr to reg_tb *1/ */

        /* set reg_target = branch target */
        uint64_t target = ins_target_addr(ins);
        int li_nr = ins_insert_before_li_d(ins, reg_target, target);
        insert_before_nr += li_nr;
        lsassert(li_nr == 4);

        /* set return value ($a0) = 0 */
        /* FIXME: return value not used */
        ins_insert_before(ins, ins_create_3(LISA_ORI, reg_a0, reg_zero, reg_zero));
        insert_before_nr++;

        /* Branch to context_switch_native_to_bt, will be modify in redirection process */
        ins_insert_before(ins, ins_b(0));
        insert_before_nr++;

        ins_remove(ins);
        ins_nr--;
    } else if (op_is_indirect_jmp(ins->op)) {
        /* 间接跳转：
         * 1. 保存目标地址到 reg_target
         * 2. 跳转到上下文切换代码（推迟到重定位再做）
         * TODO: 添加stub for jmp_glue
         */
        int rd = ins->opnd[0].val;
        int rj = ins->opnd[1].val;
        int offset16 = ins->opnd[2].val;

        /* reg_target(PC) = GR[rj] + SignExtend(off16 << 2) */
        int itemp_offset = reg_alloc_itemp();
        int li_nr = ins_insert_before_li_d(ins, itemp_offset, sign_extend(offset16 << 2, 18));
        ins_insert_before(ins, ins_create_3(LISA_ADD_D, reg_target, rj, itemp_offset));
        insert_before_nr += li_nr + 1;
        reg_free_itemp(itemp_offset);

        /* GR[rd] = PC + 4 */
        /* JIRL 会写寄存器$rd，因此这里提前保存$rd */
        if (gpr[0] != reg_zero) {
            uint64_t next_pc = ins->pc + 4;
            int li_nr = ins_insert_before_li_d(ins, rd, next_pc);
            ins_insert_before(ins, ins_create_3(LISA_ST_D, rd, reg_env, env_offset_of_gpr(cs, gpr[0])));
            insert_before_nr += li_nr + 1;
        }

        /* set return value ($a0) = 0 */
        /* FIXME: return value not used */
        ins_insert_before(ins, ins_create_3(LISA_OR, reg_a0, reg_zero, reg_zero));
        insert_before_nr++;

        /* Branch to context_switch_native_to_bt, will be modify in redirection process */
        ins_insert_before(ins, ins_b(0));
        insert_before_nr++;

        ins_remove(ins);
        ins_nr--;
    } else if (ins->op == LISA_SYSCALL || ins->op == LISA_BREAK) {
        /* FIXME: put SYSCALL and BREAK together, but BREAK not tested */
        lsassert(ins->op != LISA_BREAK);

        int itemp = reg_alloc_itemp();
        int itemp_cpu = reg_alloc_itemp();

        /* save next_pc */
        /* BUG: int li_nr = ins_insert_before_li_d(ins, itemp, ins->pc + 4); */
        int li_nr = ins_insert_before_li_d(ins, itemp, ins->pc);
        ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_of_pc(cs)));
        insert_before_nr += li_nr + 1;

        /* set exception index */
        if (ins->op == LISA_SYSCALL) {
            li_nr = ins_insert_before_li_d(ins, itemp, EXCCODE_SYS);
        } else {
            li_nr = ins_insert_before_li_d(ins, itemp, EXCCODE_BRK);
        }
        ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_exception_index(cs)));
        insert_before_nr += li_nr + 1;

        /* call cpu_loop_exit(CPUState *cpu) 
         * BUG promt: 很可能出问题
         * TODO: maybe need call cpu_loop_exit_restore(CPUState *cpu, uintptr_t pc) */
        li_nr = ins_insert_before_li_d(ins, itemp, (uint64_t)cpu_loop_exit);
        li_nr = ins_insert_before_li_d(ins, itemp_cpu, (uint64_t)cs);
        /* syscall will never return, so reg_ra is no use */
        ins_insert_before(ins, ins_create_3(LISA_OR, reg_a0, reg_zero, itemp_cpu));
        ins_insert_before(ins, ins_create_3(LISA_JIRL, reg_ra, itemp, 0));
        insert_before_nr += 2 * li_nr + 2;

        reg_free_itemp(itemp_cpu);
        reg_free_itemp(itemp);

        ins_remove(ins);
        ins_nr--;
        /* translate end */
        /* ctx->base.is_jmp = DISAS_NORETURN; */
    }

    for (int i = 0; i < insert_before_nr; ++i)
        *start = (*start)->prev;
    for (int i = 0; i < insert_after_nr; ++i)
        *end = (*end)->next;

    ins_nr += insert_before_nr + insert_after_nr;
    return ins_nr;
}

int ins_append_exit(Ins *ins, Ins **end)
{
        /* set reg_target = branch target */
        Ins *b = ins_b(0);
        ins_append(b);
        *end = b;

        uint64_t target = ins->pc + 4;
        int li_nr = ins_insert_before_li_d(b, reg_target, target);
        ins_insert_before(b, ins_create_3(LISA_ORI, reg_a0, reg_zero, reg_zero));

        return li_nr + 2;
}

/* context switch */
#include "instrument.h"
/* #include "regs.h" */
/* #include "error.h" */

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
#define FCSR_EXTRA_SPACE        (RA_EXTRA_SPACE + REG_LEN)
static void generate_context_switch_bt_to_native(void)
{
    /* allocate space on the stack */
    ins_append_3(LISA_ADDI_D, reg_sp, reg_sp, -256);
    /* save callee-saved LA registers. s0-s8, fp */
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

    /* save ra */
    ins_append_3(LISA_ST_D, reg_ra, reg_sp, RA_EXTRA_SPACE);

    /* FIXME: 目前看来只有这两个寄存器是有必要恢复的，其他都不需要了？ */
    /* arg1(a0): code_cache */
    /* arg2(a1): env */
    /* FIXME: use reg_fp to save native_addr maybe not a good idea */
    ins_append_3(LISA_OR, reg_code_ptr, reg_a0, reg_zero);
    ins_append_3(LISA_OR, reg_env, reg_a1, reg_zero);

    /* FIXME: confused */
    /* /1* save dbt FCSR *1/ */
    /* int reg_temp_fcsr = reg_s0; */
    /* ins_append_3(LISA_LD_W, reg_temp_fcsr, reg_env, lsenv_offset_of_fcsr(lsenv)); */
    /* ins_append_2(LISA_MOVGR2FCSR, reg_fcsr, reg_temp_fcsr); */

    /* /1* load native registers from env. *1/ */
    /* /1* FIXME: not implemented !! *1/ */
    /* tr_load_registers_from_env(0xff, 0x0, 0x0, options_to_save()); */

    /* jump to native code address */
    ins_append_3(LISA_JIRL, reg_zero, reg_code_ptr, 0);
}

static void generate_context_switch_native_to_bt(CPUState *cs)
{
    int reg_ret = reg_a0;
    ins_append_3(LISA_OR, reg_ret, reg_zero, reg_zero);

    /* 1. store the last executed TB (env->gpr[1], $ra) */
    lsassert(env_offset_of_last_executed_tb(cs) >= -2048 &&
            env_offset_of_last_executed_tb(cs) <= 2047);
    ins_append_3(LISA_ST_D, reg_code_ptr, reg_env,
                            env_offset_of_last_executed_tb(cs));
    /* 2. store eip (env->pc) */
    /* reg_target's value is set by branch instruction */
    lsassert(env_offset_of_pc(cs) >= -2048 &&
            env_offset_of_pc(cs) <= 2047);
    ins_append_3(LISA_ST_D, reg_target, reg_env,
                            env_offset_of_pc(cs));
    /* TODO:
     * last_executed_tb in gpr[1]
     * next_eip in gpr[2]
     * */

    /* FIXME: not implemented */
    /* /1* 3. store registers to env *1/ */
    /* tr_save_registers_to_env(0xff, 0x0, 0x0, options_to_save()); */

    /* /1* 4. restore dbt FCSR *1/ */
    /* int reg_temp_fcsr = reg_s0; */
    /* /1* save fcsr for native *1/ */
    /* ins_append_2(LISA_MOVFCSR2GR, reg_temp_fcsr, reg_fcsr); */
    /* ins_append_3(LISA_ST_W, reg_temp_fcsr, reg_env, */
    /*                       lsenv_offset_of_fcsr(lsenv)); */

    /* 5. restore ra */
    ins_append_3(LISA_LD_D, reg_ra, reg_sp, RA_EXTRA_SPACE);

    /* 6. restore callee-saved registers. s0-s8, fp */
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
    /* 7. restore sp */
    ins_append_3(LISA_ADDI_D, reg_sp, reg_sp, 256);

    /* 8. return value */
    /* reg_ret(return value) is set by branch instruction */
    ins_append_3(LISA_OR, reg_a0, reg_ret, reg_zero);
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
    generate_context_switch_bt_to_native();
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
