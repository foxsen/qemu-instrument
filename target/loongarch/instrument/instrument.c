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
    Ins *ins = NULL;
    int ins_nr = 0;
    int real_ins_nr = 0;

    TRACE trace = TRACE_alloc(pc);
    BBL bbl = BBL_alloc(pc);
    tr_data.trace = trace;

    while (1) {
        /* disasm */
        uint32_t opcode = read_opcode(cs, pc);
        ins = ins_alloc(pc);
        la_disasm_one_ins(opcode, ins);
        ins_append(ins);

        /* translate */
        Ins *start = ins, *end = ins;
        int len = ins_translate(cs, ins, &start, &end);

        if (max_insns == 1) {
            len += ins_append_exit(ins, &end);
        }

        INS pin_ins = INS_alloc(pc, opcode);
        INS_set_range(pin_ins, start, end, len);
        BBL_append_ins(bbl, pin_ins);

        /* TODO: ins_instrument */

        pc += 4;
        ins_nr++;
        real_ins_nr += len;

        if (max_insns == 1) {
            TRACE_append_bbl(trace, bbl);
            /* TODO: bbl_instrument */
            break;
        } else if (op_is_branch(ins->op) || ins->op == LISA_SYSCALL) {
            TRACE_append_bbl(trace, bbl);

            /* TODO: bbl_instrument */

            if (op_is_condition_jmp(ins->op)) {
                bbl = BBL_alloc(pc);
            } else {
                break;
            }
        }
    }
    lsassert(ins_nr <= max_insns);

    /* TODO: trace_instrument */

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
            uint64_t curr_ins_pos = (uint64_t)tb->tc.ptr + (ins_nr << 2);
            uint64_t exit_offset = context_switch_native_to_bt - curr_ins_pos;
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



extern int lmj_showtrans;

/* ins -> binary */
    /* TODO: 这些指针没搞清楚 */
    /* code_buf = tcg_ctx->code_gen_ptr; */
    /* if (unlikely((void *)s->code_ptr > s->code_gen_highwater)) { */
    /*     return -1; */
    /* } */
int la_encode(TCGContext *tcg_ctx, void* code_buf)
{
    TRANSLATION_DATA *t = &tr_data;

    /* check code_cache overflow. */
    uint64_t code_size = t->list_ins_nr * 4;
    if (code_buf + code_size >
        tcg_ctx->code_gen_buffer + tcg_ctx->code_gen_buffer_size) {
        tr_data.curr_tb = NULL;
        return -1;
    }

    int ins_nr = 0;
    uint32_t *code_ptr = code_buf;
    for (Ins *ins = t->first_ins; ins != NULL; ins = ins->next) {
        uint32_t opcode = la_assemble(ins);
        *code_ptr = opcode;
        ++code_ptr;
        ++ins_nr;
    }

    /* Debug Info:
     * Print origin_ins and translated_ins */
    if (lmj_showtrans == 1 && tr_data.trace != NULL) {
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

    lsassert(t->list_ins_nr == ins_nr);
    return ins_nr * 4;
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
