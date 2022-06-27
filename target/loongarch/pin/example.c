#include "example.h"
#include "target/loongarch/instrument/la_disasm/include/disasm.h"
#include "ins_inspection.h"

static UINT64 icount = 0;
 
VOID docount(UINT64 pc, UINT32 opcode)
{ 
    ++icount;
    char msg[128];
    la_disasm_print(opcode, msg);
    fprintf(stderr, "%lu\t0x%lx: %s\n", icount, pc, msg);
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
 
    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
