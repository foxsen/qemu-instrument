#include "example.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
#include "qemu.h"
#include "user-internals.h"
#include "strace.h"
#include "../../instrument/ins.h"
#include "../../instrument/regs.h"


VOID Trace(TRACE trace, VOID* v);
VOID Fini(INT32 code, VOID* v);

static UINT64 icount = 0;


VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &icount, BBL_NumIns(bbl), false);
    }
}

VOID Fini(INT32 code, VOID* v)
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

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
