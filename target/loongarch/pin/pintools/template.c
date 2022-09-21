#include "example.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
#include "qemu.h"
#include "user-internals.h"
#include "strace.h"
#include "../../instrument/ins.h"
#include "../../instrument/regs.h"


/* INS */
static UINT64 icount = 0;

static VOID show_ins(UINT64 pc, UINT32 opcode)
{
    ++icount;

    char msg[128];
    Ins ins;
    la_disasm(opcode, &ins);
    sprint_ins(&ins, msg);
    fprintf(stderr, "thread %d: %lu\tpc: 0x%lx\t%s\n", PIN_ThreadId(), icount, pc, msg);
}

static VOID Instruction(INS ins, VOID* v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)show_ins, IARG_UINT64, ins->pc, IARG_UINT64, ins->opcode, IARG_END);
}



/* BBL */
static uint64_t bbl_exec_nr = 0;
static uint64_t ins_exec_nr = 0;

static VOID Trace(TRACE trace, VOID* v)
{
    BOOL is_atomic = false;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        /* inline add */
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &bbl_exec_nr, 1, is_atomic);
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &ins_exec_nr, bbl->nr_ins, is_atomic);
    }
}



/* cpu_exec */
static VOID cpu_exec_enter(CPUState *cpu, TranslationBlock *tb)
{
}

static VOID cpu_exec_exit(CPUState *cpu, TranslationBlock *last_tb, int tb_exit)
{
}



/* syscall */
static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    CPULoongArchState * env= ctxt->env;
    uint64_t syscall_nr = env->gpr[11];
    /* print_syscall(env, num, env->gpr[4], env->gpr[5], env->gpr[6], env->gpr[7], env->gpr[8], env->gpr[9]); */
    fprintf(stderr, "Thread %u: SYSCALL: %s (a0-a6: %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n", threadIndex, syscall_name(syscall_nr), ctxt->env->gpr[4], ctxt->env->gpr[5], ctxt->env->gpr[6], ctxt->env->gpr[7], ctxt->env->gpr[8], ctxt->env->gpr[9], ctxt->env->gpr[10]);
}

static VOID syscall_exit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    /* 提前知道 syscall_nr 才能 ... */
    CPULoongArchState * env= ctxt->env;
    fprintf(stderr, "Thread %u: SYSCALL_RET: %ld\n", threadIndex, env->gpr[4]);
}



static VOID Init(VOID)
{
}

static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "Total Ins Count: %lu\n", icount);
    fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INS/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int ins_instru(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    Init();

    /* print_ins */
    INS_AddInstrumentFunction(Instruction, 0);
    /* bbl_count */
    TRACE_AddInstrumentFunction(Trace, 0);

    /* syscall trace */
    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddSyscallExitFunction(syscall_exit, 0);

    PIN_AddCpuExecEnterFunction(cpu_exec_enter, 0);
    PIN_AddCpuExecExitFunction(cpu_exec_exit, 0);

    /* result */
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
