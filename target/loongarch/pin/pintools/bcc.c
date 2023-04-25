#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../../instrument/decoder/ins.h"
#include "../ins_inspection.h"

static UINT64 b_cnt = 0;
static UINT64 b_exit_cnt = 0;   /* also count bcc exit */

static UINT64 bcc_forward_cnt = 0;
static UINT64 bcc_backward_cnt = 0;
static UINT64 bcc_fallthrough_cnt = 0;
/* static UINT64 bcc_exit_cnt = 0; */

static UINT64 jirl_cnt = 0;
static UINT64 jirl_exit_cnt = 0;

static UINT64 cpu_enter_cnt = 0;
static UINT64 cpu_exit_cnt = 0;
static UINT64 syscall_exit_cnt = 0;

static VOID count_bcc(LA_OPCODE op, BOOL branch_taken, BOOL branch_backward) {
    if (branch_taken) {
        if (branch_backward) {
            ++bcc_backward_cnt;
        } else {
            ++bcc_forward_cnt;
        }
    } else {
        ++bcc_fallthrough_cnt;
    }
}

static VOID Trace(TRACE trace, VOID* v)
{
    BOOL is_atomic = false;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        INS last_ins = bbl->ins_tail;
        IR2_OPCODE op = last_ins->origin_ins->op;
        if (op_is_indirect_branch(op)) {
            BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &jirl_cnt, 1, is_atomic);
        } else if (op_is_direct_branch(op)) {
            if (op_is_condition_branch(op)) {
                /* BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &bcc_exec_cnt, 1, is_atomic); */
                BOOL branch_backward = false;
                if (LISA_BEQZ<= op && op <= LISA_BCNEZ) {
                    if (last_ins->origin_ins->opnd[1].val < 0) {
                        branch_backward = true;
                    }
                } else if (LISA_BEQ <= op && op <= LISA_BGEU) {
                    if (last_ins->origin_ins->opnd[2].val < 0) {
                        branch_backward = true;
                    }
                } else {
                    fprintf(stderr, "not supported bcc op");
                }
                INS_InsertCall(last_ins, IPOINT_BEFORE, (AFUNPTR)count_bcc, IARG_UINT64, op, IARG_BRANCH_TAKEN, IARG_BOOL, branch_backward, IARG_END);
            } else {
                BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &b_cnt, 1, is_atomic);
            }
        } else if (!op_is_syscall(op)) {
            fprintf(stderr, "not supported bbl tail op");
            print_ins(last_ins->origin_ins);
        }
    }
}



/* cpu_exec */
static VOID cpu_exec_enter(CPUState *cpu, TranslationBlock *tb)
{
    ++cpu_enter_cnt;
}

static VOID cpu_exec_exit(CPUState *cpu, TranslationBlock *last_tb, int tb_exit)
{
    ++cpu_exit_cnt;
    if (last_tb == NULL) {
        ++jirl_exit_cnt;
    } else {
        ++b_exit_cnt;
    }
}

static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    ++syscall_exit_cnt;
}

static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "B %lu\n", b_cnt);
    fprintf(stderr, "BCC_forward %lu\n", bcc_forward_cnt);
    fprintf(stderr, "BCC_backward %lu\n", bcc_backward_cnt);
    fprintf(stderr, "BCC_fallthrough %lu\n", bcc_fallthrough_cnt);
    fprintf(stderr, "JIRL %lu\n", jirl_cnt);
    fprintf(stderr, "B_exit %lu\n", b_exit_cnt);
    fprintf(stderr, "JIRL_exit %lu\n", jirl_exit_cnt);
    fprintf(stderr, "SYSCALL_exit %lu\n", syscall_exit_cnt);
    fprintf(stderr, "B_miss_rate %f\n", (double)b_exit_cnt / (b_cnt + bcc_forward_cnt + bcc_backward_cnt + bcc_fallthrough_cnt));
    fprintf(stderr, "JIRL_miss_rate %f\n", (double)jirl_exit_cnt / jirl_cnt);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddCpuExecEnterFunction(cpu_exec_enter, 0);
    PIN_AddCpuExecExitFunction(cpu_exec_exit, 0);

    /* result */
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
