#include "example.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
#include "qemu.h"
#include "user-internals.h"
#include "strace.h"
#include "../../instrument/ins.h"
#include "../../instrument/regs.h"

static UINT64 icount1 = 0;
static UINT64 icount2 = 0;

VOID docount(UINT64 pc, UINT32 opcode);
VOID docount(UINT64 pc, UINT32 opcode)
{
    /* icount */
    ++icount1;
    fprintf(stderr, "%lx\n", pc);

    /* print ins */
    /* char msg[128]; */
    /* Ins ins; */
    /* la_disasm(opcode, &ins); */
    /* sprint_ins(&ins, msg); */
    /* /1* fprintf(stderr, "thread %d: %lu\tpc: 0x%lx\t%s\n", PIN_ThreadId(), icount1, pc, msg); *1/ */
    /* fprintf(stderr, "%lu\tpc: 0x%lx\t%s\n", icount1, pc, msg); */
}

static VOID Instruction(INS ins, VOID* v)
{
    /* icount */
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_UINT64, ins->pc, IARG_UINT64, ins->opcode, IARG_END);
}



static uint64_t bbl_exec_nr = 0;
static uint64_t ins_exec_nr = 0;
static VOID bbl_statistic(uint64_t ins_nr) {
    ++bbl_exec_nr;
    ins_exec_nr += ins_nr;
}

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        /* BBL count */
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)bbl_statistic, IARG_UINT64, bbl->nr_ins, IARG_END);
    }
}


#include "../../instrument/instrument.h"
static VOID Fini(INT32 code, VOID* v)
{
    /* fprintf(stderr, "Ins Count1: %lu\n", icount1); */
    /* fprintf(stderr, "Ins Count2: %lu\n", icount2); */

    
    /* fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INSs/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr); */
}
 
INT32 Usage(void)
{
    return -1;
}
 
int ins_instru(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    /* Init(); */

    INS_AddInstrumentFunction(Instruction, 0);

    /* TRACE_AddInstrumentFunction(Trace, 0); */

    /* PIN_AddSyscallEntryFunction(syscall_enter, 0); */
    /* PIN_AddSyscallExitFunction(syscall_exit, 0); */

    /* PIN_AddCpuExecEnterFunction(cpu_exec_enter, 0); */
    /* PIN_AddCpuExecExitFunction(cpu_exec_exit, 0); */
 
    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
