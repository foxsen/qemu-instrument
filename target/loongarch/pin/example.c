#include "example.h"
#include "target/loongarch/instrument/la_disasm/include/disasm.h"
#include "ins_inspection.h"
#include "qemu.h"
#include "user-internals.h"
#include "strace.h"

static UINT64 icount = 0;
 
VOID docount(UINT64 pc, UINT32 opcode)
{ 
    ++icount;
    char msg[128];
    la_disasm_print(opcode, msg);
    fprintf(stderr, "thread %d: %lu\t0x%lx: %s\n", PIN_ThreadId(), icount, pc, msg);
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



VOID Instruction(INS ins, VOID* v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_UINT64, ins->pc, IARG_UINT64, ins->opcode, IARG_END);
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

VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "Count: %lu\n", icount);
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
