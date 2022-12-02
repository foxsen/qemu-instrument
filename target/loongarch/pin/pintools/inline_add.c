#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"


/* INS */
/* static UINT64 icount = 0; */

/* static VOID Instruction(INS ins, VOID* v) */
/* { */
/*     INS_InsertInlineAdd(ins, IPOINT_BEFORE, &icount, 1); */
/* } */


/* 每个BBL插1~2个inline_add的性能差不多，插3个以上性能又会下降。也许和微架构有关。 */

/* BBL */
static uint64_t bbl_exec_nr = 0;
static uint64_t ins_exec_nr = 0;
/* static uint64_t ins_exec_nr2 = 0; */
/* static uint64_t ins_exec_nr3 = 0; */

static VOID Trace(TRACE trace, VOID* v)
{
    BOOL is_atomic = false;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &bbl_exec_nr, 1, is_atomic);
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &ins_exec_nr, bbl->nr_ins, is_atomic);
        /* BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &ins_exec_nr2, bbl->nr_ins, is_atomic); */
        /* BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &ins_exec_nr3, bbl->nr_ins, is_atomic); */
    }
}


static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INS/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr);
    /* fprintf(stderr, "INS: %ld\n", ins_exec_nr); */
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    /* INS_AddInstrumentFunction(Instruction, 0); */

    /* bbl_count */
    TRACE_AddInstrumentFunction(Trace, 0);

    /* result */
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
