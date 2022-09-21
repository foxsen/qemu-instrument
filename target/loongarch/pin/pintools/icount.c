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
    /* print ins */
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
static VOID bbl_statistic(uint64_t ins_nr) {
    ++bbl_exec_nr;
    ins_exec_nr += ins_nr;
}

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)bbl_statistic, IARG_UINT64, BBL_NumIns(bbl), IARG_END);
    }
}





static VOID Init(VOID)
{
}

static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "Total Ins Count: %lu\n", icount);
    /* fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INSs/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr); */
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int ins_instru(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    Init();

    INS_AddInstrumentFunction(Instruction, 0);

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
