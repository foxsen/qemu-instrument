#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"

static UINT64 icount = 0;
static UINT64 ircount = 0;
static UINT64 iwcount = 0;

static VOID ins_read(UINT64 pc, UINT64 addr)
{
    ++ircount;
    fprintf(stderr, "[R] pc=0x%lx, addr=0x%lx\n", pc, addr);
}

static VOID ins_write(UINT64 pc, UINT64 addr)
{
    ++iwcount;
    fprintf(stderr, "[W] pc=%lx, addr=%lx\n", pc, addr);
}

static VOID Instruction(INS ins, VOID* v)
{
    if (INS_IsMemoryRead(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ins_read, IARG_UINT64, ins->pc, IARG_MEMORYOP_EA, 0, IARG_END);
    } else if (INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ins_write, IARG_UINT64, ins->pc, IARG_MEMORYOP_EA, 0, IARG_END);
    }
}

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &icount, BBL_NumIns(bbl), false);
    }
}

static VOID Init(VOID)
{
}

static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "total_ins=%ld, read_cnt=%ld(%.2f%%), write_cnt=%ld(%.2f%%)\n", icount, ircount, (double)ircount/icount*100, iwcount, (double)iwcount/icount*100);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    Init();

    INS_AddInstrumentFunction(Instruction, 0);

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
