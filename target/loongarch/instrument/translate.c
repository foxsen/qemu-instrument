#include "translate.h"
#include "regs.h"
#include "error.h"
#include "bitopts.h"

/* for cpu_loop_exit */
#include "exec/exec-all.h"
extern int lmj_debug;
extern int fullregs;
extern int enable_jmp_cache;

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
    int insert_before_nr = 0;
    int insert_after_nr = 0;
    bool enable_tb_link = ((tb_cflags(tr_data.curr_tb) & CF_NO_GOTO_TB) == 0);

    /* FIXME: 目前指令的寄存器操作数是读/写还不确定，因此一律ld + st (对于st指令，dest均为读，其中sc读后还要会写) */
    /* FIXME: we can do better(延迟释放已分配的itemp) */
    /* Native reg is in the memory
     * so when:
     * - ins read gpr: load it to itemp;
     * - ins write gpr: write back to memory.
     */
    /* 1. record regs used, maping to itemp */
    /* gpr[4] 保存四个操作数用到的原寄存器 */
    int gpr[4] = {-1, -1, -1, -1};
    for (int i = 0; i < ins->opnd_count; i++) {
        /* 忽略 reg_zero */
        if (opnd_is_gpr(ins, i) && ins->opnd[i].val != reg_zero) {
            int reg = ins->opnd[i].val;
            int reg_itemp = map_native_reg_to_itemp(reg);
            gpr[i] = reg;
            /* 用临时寄存器替换指令中所使用的原始寄存器 */
            ins->opnd[i].val = reg_itemp;
        }
    }

    /* 2. add LD/ST ins around origin ins */
    if (!fullregs && is_ir2_reg_access_type_valid(ins)) {
        for (int i = 0; i < 4; ++i) {
            int reg = gpr[i];
            if (reg != -1) {
                int reg_itemp = map_native_reg_to_itemp(reg);
                if (opnd_is_gpr_read(ins, i) || opnd_is_gpr_readwrite(ins, i)) {
                    ins_insert_before(ins, ins_create_3(LISA_LD_D, reg_itemp, reg_env, env_offset_of_gpr(cs, reg)));
                    insert_before_nr++;
                }
                if (opnd_is_gpr_write(ins, i) || opnd_is_gpr_readwrite(ins, i)) {
                    ins_insert_after(ins, ins_create_3(LISA_ST_D, reg_itemp, reg_env, env_offset_of_gpr(cs, reg)));
                    insert_after_nr++;
                }
            }
        }
    } else {
        /* remove dup regs */
        for (int i = 3; i >= 0; --i) {
            for (int j = i - 1; j >= 0; --j) {
                if (gpr[i] == gpr[j]) {
                    gpr[i] = -1;
                    break;
                }
            }
        }
        if (lmj_debug) {
            char msg[16];
            op_print(ins->op, msg);
            fprintf(stderr, "Unimplemented insn opnd access type: %s\n", msg);
        }
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
         * FIXME: BUG: 不写0会运行会出错
         * */
        int rd = ins->opnd[0].val;
        ins_insert_before(ins, ins_create_3(LISA_OR, rd, reg_zero, reg_zero));
        insert_before_nr++;

        ins_remove(ins);
        ins_nr--;
    } else if (ins->op == LISA_PCADDI || ins->op == LISA_PCADDU12I || ins->op == LISA_PCADDU18I || ins->op == LISA_PCALAU12I) {
        /* PCADD 系列指令 */
        int itemp_pc = reg_alloc_itemp();
        int itemp_offset = reg_alloc_itemp();
        int li_nr = ins_insert_before_li_d(ins, itemp_pc, ins->pc);
        insert_before_nr += li_nr;
        /* TODO: can use less insn to achieve it */
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
        tr_data.is_jmp = TRANS_NORETURN;

        int bcc_jmp_over = enable_tb_link ? 11 : 7;
        switch (ins->op) {
            case LISA_BEQZ:
                /* FIXME: maybe we can calc the magic number and patch this ins later */
                /* magic number 11: we will insert additional 10 ins before next ins */
                ins_insert_before(ins, ins_create_2(LISA_BNEZ, ins->opnd[0].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BNEZ:
                ins_insert_before(ins, ins_create_2(LISA_BEQZ, ins->opnd[0].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BCEQZ:
                ins_insert_before(ins, ins_create_2(LISA_BCNEZ, ins->opnd[0].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BCNEZ:
                ins_insert_before(ins, ins_create_2(LISA_BCEQZ, ins->opnd[0].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BEQ:
                ins_insert_before(ins, ins_create_3(LISA_BNE, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BNE:
                ins_insert_before(ins, ins_create_3(LISA_BEQ, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BLT:
                ins_insert_before(ins, ins_create_3(LISA_BGE, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BGE:
                ins_insert_before(ins, ins_create_3(LISA_BLT, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BLTU:
                ins_insert_before(ins, ins_create_3(LISA_BGEU, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            case LISA_BGEU:
                ins_insert_before(ins, ins_create_3(LISA_BLTU, ins->opnd[0].val, ins->opnd[1].val, bcc_jmp_over));
                insert_before_nr++;
                break;
            default:
                break;
        }

        if (ins->op == LISA_BL) {
            /* save return address: GR[1] = PC + 4 */
            uint64_t next_pc = ins->pc + 4;
            int itemp_ra = reg_alloc_itemp();
            insert_before_nr += ins_insert_before_li_d(ins, itemp_ra, next_pc);
            ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp_ra, reg_env, env_offset_of_gpr(cs, reg_ra)));
            insert_before_nr++;
            reg_free_itemp(itemp_ra);
        }

        /* tb_link: fallthrough, this B ins will be patched when tb_link */
        if (enable_tb_link) {
            Ins *b = ins_b(1);
            tr_data.jmp_ins[0] = b;
            ins_insert_before(ins, b);
            insert_before_nr++;
        }

        /* set reg_target = branch target */
        insert_before_nr += ins_insert_before_li_d(ins, reg_target, ins_target_addr(ins));

        /* set return value */
        if (enable_tb_link) {
            /* tb_link: $a0 = (tb | slot_index) */
            insert_before_nr += ins_insert_before_li_d(ins, reg_a0, ((uint64_t)tr_data.curr_tb | 0));
        } else {
            /* $a0 = 0 */
            ins_insert_before(ins, ins_create_3(LISA_ORI, reg_a0, reg_zero, reg_zero));
            insert_before_nr++;
        }

        /* Branch to context_switch_native_to_bt, will be modify in relocation process */
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
        tr_data.is_jmp = TRANS_NORETURN;

        int rd = ins->opnd[0].val;
        int rj = ins->opnd[1].val;
        int offset16 = ins->opnd[2].val;

        /* reg_target(PC) = GR[rj] + SignExtend(off16 << 2) */
        int itemp_off = reg_alloc_itemp();
        int li_nr = ins_insert_before_li_d(ins, itemp_off, sign_extend(offset16 << 2, 18));
        ins_insert_before(ins, ins_create_3(LISA_ADD_D, reg_target, rj, itemp_off));
        insert_before_nr += li_nr + 1;
        reg_free_itemp(itemp_off);

        /* GR[rd] = PC + 4 */
        /* JIRL 会写寄存器$rd，因此这里提前保存$rd */
        if (gpr[0] != -1 && gpr[0] != reg_zero) {
            uint64_t next_pc = ins->pc + 4;
            int li_nr = ins_insert_before_li_d(ins, rd, next_pc);
            ins_insert_before(ins, ins_create_3(LISA_ST_D, rd, reg_env, env_offset_of_gpr(cs, gpr[0])));
            insert_before_nr += li_nr + 1;
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
            insert_before_nr += li_nr + 11;
            reg_free_itemp(itemp_tb);
            reg_free_itemp(itemp);
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
        tr_data.is_jmp = TRANS_NORETURN;

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
        } else if (ins->op == LISA_BREAK){
            li_nr = ins_insert_before_li_d(ins, itemp, EXCCODE_BRK);
        }
        ins_insert_before(ins, ins_create_3(LISA_ST_D, itemp, reg_env, env_offset_of_exception_index(cs)));
        insert_before_nr += li_nr + 1;

        /* call cpu_loop_exit(CPUState *cpu) 
         * BUG promt: 很可能出问题
         * TODO: maybe need call (see) cpu_loop_exit_restore(CPUState *cpu, uintptr_t pc) */
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
    }

    Ins *start = ins, *end = ins;
    /* TODO: a ugly fix for ins被remove的情况 */
    if (ins_nr == 0) {
        end = end->prev;
    }

    for (int i = 0; i < insert_before_nr; ++i)
        start = start->prev;
    for (int i = 0; i < insert_after_nr; ++i)
        end = end->next;
    ins_nr += insert_before_nr + insert_after_nr;
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
        /* tb_link: fallthrough, this B ins will be patched when tb_link */
        Ins *b = ins_b(1);
        tr_data.jmp_ins[index] = b;
        ins_insert_after(pin_ins->last_ins, b);
        ++num;

        end = ins_b(0);
        ins_insert_after(b, end);
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
    /* arg1(a0): env */
    /* arg2(a1): code_cache */
    /* FIXME: use reg_fp to save native_addr maybe not a good idea */
    ins_append_3(LISA_OR, reg_env, reg_a0, reg_zero);
    ins_append_3(LISA_OR, reg_code_ptr, reg_a1, reg_zero);

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
