#include "types.h"
#include "la_disasm/include/assemble.h"
#include "la_disasm/include/disasm.h"
#include "ins.h"
#include "instrument.h"
#include "regs.h"
#include "env.h"
#include "error.h"
#include "pin_types.h"
#include "translate.h"
#include "../pin/pin_state.h"


static inline uint32_t read_opcode(CPUState *cs, uint64_t pc)
{
    CPULoongArchState *env = cs->env_ptr;
    return cpu_ldl_code(env, pc);
}

/* binary -> ins_list
 * generate TRACE/BBL/INS
 */
int la_decode(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    addr_t start_pc = tb->pc;
    addr_t pc = start_pc;
    Ins *la_ins = NULL;
    int ins_nr = 0;
    int real_ins_nr = 0;

    TRACE trace = TRACE_alloc(pc);
    BBL bbl = BBL_alloc(pc);
    tr_data.trace = trace;

    while (1) {
        /* disasm */
        uint32_t opcode = read_opcode(cs, pc);
        la_ins = ins_alloc(pc);
        la_disasm_one_ins(opcode, la_ins);
        /* ins_append(la_ins); */  /* FIXME: maybe no use anymore */

        /* 注：现在 INS_tanslate, INS_instrument, INS_append_exit 内部都会正确更新 ins->nr_ins_real */
        INS ins = INS_alloc(pc, opcode, la_ins);
        INS_translate(cs, ins);
        INS_instrument(ins);
        ++ins_nr;
        /* if (ins_nr == max_insns) { */
        /* 条件跳转也作为trace结束 */
        if (ins_nr == max_insns || op_is_condition_jmp(la_ins->op)) {
            INS_append_exit(ins);
        }
        BBL_append_ins(bbl, ins);

        pc += 4;
        real_ins_nr += ins->nr_ins_real;

#ifdef CONFIG_LMJ_DEBUG
        int c1 = 0;
        for (Ins *i = ins->first_ins; i != NULL; i = i->next) {
            c1++;
            if (i->next == NULL && i != ins->last_ins) {
                fprintf(stderr, "assert fail\n");
                char msg[128];
                ins_print(ins->origin_ins, msg);
                fprintf(stderr, "origin: %p, %s\n", ins->origin_ins, msg);
                ins_print(i, msg);
                fprintf(stderr, "i: %p, %s\n", i, msg);
                ins_print(ins->last_ins, msg);
                fprintf(stderr, "ins->last_ins: %p, %s\n", ins->last_ins, msg);
                fprintf(stderr, "ins_real_nr: %d,\tc1: %d\n", ins->nr_ins_real, c1);
                /* move this to INS_dump() */
                for (Ins *in = ins->first_ins; in != NULL; in = in->next) {
                    ins_print(in, msg);
                    fprintf(stderr, "%p: %08x\t%s\n", in, opcode, msg);
                }
                lsassert(0);
            }
        }
        lsassertm(c1 == ins->nr_ins_real, "c1: %d, read: %d", c1, ins->nr_ins_real);
#endif

        if (ins_nr == max_insns || op_is_branch(la_ins->op) || la_ins->op == LISA_SYSCALL) {
            /* 条件跳转也作为trace结束 */
            TRACE_append_bbl(trace, bbl);
            break;
            /* if (op_is_condition_jmp(la_ins->op)) { */
            /*     bbl = BBL_alloc(pc); */
            /* } else { */
            /*     break; */
            /* } */
        }
    }
    lsassertm(ins_nr <= max_insns, "tb ins_nr >= max_insns(%d)\n", max_insns);

    TRACE_instrument(trace);

    tr_data.first_ins = trace->bbl_head->ins_head->first_ins;
    tr_data.last_ins = trace->bbl_tail->ins_tail->last_ins;
    /* real_ins_nr没有计算插桩加入指令数，现在用 ins_insert 系列指令来维护 tr_data.list_ins_nr */
    /* lsassertm(tr_data.list_ins_nr == real_ins_nr, "list_ins_nr(%d) != real_ins_nr(%d), trace->ins_nr=%d\n", tr_data.list_ins_nr, real_ins_nr, trace->nr_ins); */
    /* tr_data.list_ins_nr = real_ins_nr; */

    /* The disas_log hook may use these values rather than recompute.  */
    tb->size = pc - start_pc;
    tb->icount = ins_nr;

    return ins_nr;
}


void la_relocation(CPUState *cs, TranslationBlock *tb)
{
    /* 跳转指令重定向
     * FIXME：目前假设所有的 B 0 指令都是要跳转到 context_switch_native_to_bt 
     */
    int ins_nr = 0;
    TRANSLATION_DATA *t = &tr_data;
    /* TranslationBlock *tb = t->curr_tb; */

    for (Ins *ins = t->first_ins; ins != NULL; ins = ins->next) {
        if (ins->op == LISA_B && ins->opnd[0].val == 0x0) {
            uint64_t cur_ins_pos = (uint64_t)tb->tc.ptr + (ins_nr << 2);
            uint64_t exit_offset = context_switch_native_to_bt - cur_ins_pos;
            ins->opnd[0].val = exit_offset >> 2;
        }
        ins_nr++;
    }

    /* TODO: handle segv scenario */
    /* 伪代码: */
    /* for (i = 0; i < ir1_nr; ++i) { */
    /*     tcg_ctx->gen_insn_data[i][0] = pir1->info->address; */
    /*     tcg_ctx->gen_insn_data[i][1] = 0; */
    /*     translate_one_ins(ins[i]); */
    /*     tcg_ctx->gen_insn_end_off[i] = (lsenv->tr_data->real_ir2_inst_num)<<2; */
    /* } */
}



extern int showtrans;

/* ins -> binary */
    /* TODO: 这些code-cache指针没搞清楚 */
    /* code_buf = tcg_ctx->code_gen_ptr; */
    /* if (unlikely((void *)s->code_ptr > s->code_gen_highwater)) { */
    /*     return -1; */
    /* } */
int la_encode(TCGContext *tcg_ctx, void* code_buf)
{
    TRANSLATION_DATA *t = &tr_data;
    uint64_t code_size = tr_data.list_ins_nr * 4;

    /* check code_cache overflow. */
    if (code_buf + code_size >
        tcg_ctx->code_gen_buffer + tcg_ctx->code_gen_buffer_size) {
        tr_data.curr_tb = NULL;
        return -1;
    }

    int ins_nr = 0;
    uint32_t *code_ptr = code_buf;
    if (tr_data.trace == NULL) {
        /* FIXME: gen_prologue 目前用这种方式来encode */
        /* FIXed: 其实现在普通的tb也能这样encode */
        for (Ins *ins = t->first_ins; ins != NULL; ins = ins->next) {
            uint32_t opcode = la_assemble(ins);
            *code_ptr = opcode;
            ++code_ptr;
            ++ins_nr;
        }
    }
    else {
        for (BBL bbl = tr_data.trace->bbl_head; bbl != NULL; bbl = bbl->next) {
            for (INS ins = bbl->ins_head; ins != NULL; ins = ins->next) {
                Ins *la_ins = ins->first_ins;
                while (la_ins) {
                    uint32_t opcode = la_assemble(la_ins);
                    *code_ptr = opcode;
                    ++code_ptr;
                    ++ins_nr;

                    if (la_ins == ins->last_ins)
                        break;
                    la_ins = la_ins->next;
                }
            }
        }
    }

#ifdef CONFIG_LMJ_DEBUG
    /* Print origin_ins and translated_ins */
    if (showtrans == 1 && tr_data.trace != NULL) {
        fprintf(stderr, "\n==== TB_ENCODE ====\n");
        char ins_info[128];
        uint32_t *pc = code_buf;
        for (BBL bbl = tr_data.trace->bbl_head; bbl != NULL; bbl = bbl->next) {
            for (INS ins = bbl->ins_head; ins != NULL; ins = ins->next) {
                la_disasm_print(ins->opcode, ins_info);
                fprintf(stderr, "0x%-16lx: %08x\t%s\n", ins->pc, ins->opcode, ins_info);

                fprintf(stderr, "------------------\n");
                Ins *inst = ins->first_ins;
                while (inst) {
                    uint32_t opcode = la_assemble(inst);
                    la_disasm_print(opcode, ins_info);
                    fprintf(stderr, "%-18p: %08x\t%s\n", pc, opcode, ins_info);

                    ++pc;
                    if (inst == ins->last_ins)
                        break;
                    inst = inst->next;
                }
                fprintf(stderr, "------------------\n");
            }
        }
        lsassert(pc == code_ptr);
        fprintf(stderr, "===================\n");
    }
#endif

    lsassertm(t->list_ins_nr == ins_nr, "t->list_ins_nr(%d) != ins_nr(%d)\n", t->list_ins_nr, ins_nr);
    return ins_nr;
}


/* int tr_translate_tb(CPUState *cs, struct TranslationBlock *tb, int max_ins) */
/* { */
/*     tr_init(tb); */
/*     /1* 1. gen_ins_list *1/ */
/*     // todo */
/*     la_decode(cs, tb, max_ins); */

/*     /1* 2. process *1/ */
/*     la_redirection(cs, tb); */

/*     /1* 3. assemble *1/ */
/*     void *code_buf = tb->tc.ptr; */
/*     int code_size = tr_data.list_ins_nr * 4; */
/*     lsassert(code_size > 0); */

/*     /1* check code_cache overflow. *1/ */
/*     if (code_buf + code_size > */
/*         tcg_ctx->code_gen_buffer + tcg_ctx->code_gen_buffer_size) { */
/*         tr_data.curr_tb = NULL; */
/*         return -1; */
/*     } */

/*     int ins_nr = 0; */
/*     ins_nr = la_encode(code_cache); */

/*     tr_fini(); */

/*     return ins_nr; */
/* } */

/* int la_init() */
/* { */
/*     la_gen_prologue(code_cache); */
/*     la_gen_epilogue(code_cache); */
/* } */
