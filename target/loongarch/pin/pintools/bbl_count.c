#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
#include <assert.h>

/* BBL */
static uint64_t bbl_exec_nr = 0;
static uint64_t ins_exec_nr = 0;
static UINT64 icount = 0;
static VOID bbl_statistic(uint64_t ins_nr) {
    ++bbl_exec_nr;
    ins_exec_nr += ins_nr;
}

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)bbl_statistic, IARG_UINT64, BBL_NumIns(bbl), IARG_END);
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &icount, BBL_NumIns(bbl), false);
    }
}


static VOID Fini(INT32 code, VOID* v)
{
    assert(ins_exec_nr == icount);
    fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INSs/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
