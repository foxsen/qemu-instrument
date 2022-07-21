#include "example.h"
#include "../instrument/decoder/disasm.h"
#include "../instrument/decoder/la_print.h"
#include "ins_inspection.h"
#include "qemu.h"
#include "user-internals.h"
#include "strace.h"
#include "../instrument/ins.h"

static UINT64 icount = 0;
VOID docount(UINT64 pc, UINT32 opcode)
{ 
    ++icount;
    char msg[128];
    Ins ins;
    la_disasm(opcode, &ins);
    sprint_ins(&ins, msg);
    fprintf(stderr, "thread %d: %lu\tpc: 0x%lx\t%s\n", PIN_ThreadId(), icount, pc, msg);
}

UINT64 gpr_cnt[32] = {0};
VOID gpr_frequency(UINT64 a, UINT64 b, UINT64 c, UINT64 d)
{
    if (a < 32) ++gpr_cnt[a];
    if (b < 32) ++gpr_cnt[b];
    if (c < 32) ++gpr_cnt[c];
    if (d < 32) ++gpr_cnt[d];
}

VOID Instruction(INS ins, VOID* v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_UINT64, ins->pc, IARG_UINT64, ins->opcode, IARG_END);

    /* Statistic gpr access frequency */
    /* int gpr[4] = {-1, -1, -1, -1}; */
    /* for (int i = 0; i < 4; ++i) { */
    /*     if (opnd_is_gpr(ins->origin_ins, i)) { */
    /*         gpr[i] = ins->origin_ins->opnd[i].val; */
    /*     } */
    /* } */
    /* INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)gpr_frequency, */
    /*         IARG_UINT64, gpr[0], */ 
    /*         IARG_UINT64, gpr[1], */
    /*         IARG_UINT64, gpr[2], */
    /*         IARG_UINT64, gpr[3], */
    /*         IARG_END); */
}

VOID docount2(UINT32 c)
{
    icount += c;
    fprintf(stderr, "docount2: %lu\n", icount);
}

VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)docount2, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
    }
}

VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    CPULoongArchState * env= ctxt->env;
    uint64_t num = env->gpr[11];
    /* print_syscall(env, num, env->gpr[4], env->gpr[5], env->gpr[6], env->gpr[7], env->gpr[8], env->gpr[9]); */
    fprintf(stderr, "thread %u: syscall %s(%lu) a0-a6: %lx, %lx, %lx, %lx, %lx, %lx, %lx\n", threadIndex, syscall_name(num), num, ctxt->env->gpr[4], ctxt->env->gpr[5], ctxt->env->gpr[6], ctxt->env->gpr[7], ctxt->env->gpr[8], ctxt->env->gpr[9], ctxt->env->gpr[10]);
}

VOID syscall_exit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    /* 提前知道 syscall_nr 才能 ... */
    CPULoongArchState * env= ctxt->env;
    fprintf(stderr, "thread %u: syscall_ret: %ld\n", threadIndex, env->gpr[4]);
}

VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "Ins Count: %lu\n", icount);
    fprintf(stderr, "gpr Count:\n");
    for (int i = 0; i < 32; ++i) {
        fprintf(stderr, "r%d: %lu\n", i, gpr_cnt[i]);
    }
}
 
INT32 Usage(void)
{
    return -1;
}
 
int ins_instru(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    INS_AddInstrumentFunction(Instruction, 0);

    /* TRACE_AddInstrumentFunction(Trace, 0); */

    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddSyscallExitFunction(syscall_exit, 0);
 
    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
