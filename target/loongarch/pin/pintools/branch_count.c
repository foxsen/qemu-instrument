#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../../instrument/decoder/ins.h"
#include "../ins_inspection.h"


/* INS */
static UINT64 jirl_count = 0;
static UINT64 b_count = 0;
static UINT64 bcc_count = 0;

static VOID Instruction(INS ins, VOID* v)
{
    if (INS_IsIndirectControlFlow(ins)) {
        INS_InsertInlineAdd(ins, IPOINT_BEFORE, &jirl_count, 1, false);
    } else if (INS_IsDirectBranch(ins)) {
        if (op_is_condition_branch(ins->origin_ins->op)) {
            INS_InsertInlineAdd(ins, IPOINT_BEFORE, &bcc_count, 1, false);
        } else {
            INS_InsertInlineAdd(ins, IPOINT_BEFORE, &b_count, 1, false);
        }
    }
}



/* BBL */
static uint64_t bbl_nr = 0;
static uint64_t ins_nr = 0;

static VOID Trace(TRACE trace, VOID* v)
{
    BOOL is_atomic = false;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &bbl_nr, 1, is_atomic);
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &ins_nr, bbl->nr_ins, is_atomic);
    }
}


static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "JIRL=%ld(%f), B=%ld(%f), BCC=%ld(%f)\n", 
            jirl_count, (double)jirl_count / ins_nr, 
            b_count, (double)b_count / ins_nr,
            bcc_count, (double)bcc_count / ins_nr);
    fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INS/BBL\n", bbl_nr, ins_nr, (double)ins_nr / bbl_nr);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    INS_AddInstrumentFunction(Instruction, 0);

    /* bbl_count */
    TRACE_AddInstrumentFunction(Trace, 0);

    /* result */
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
