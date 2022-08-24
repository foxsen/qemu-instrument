#include "example.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
#include "qemu.h"
#include "user-internals.h"
#include "strace.h"
#include "../../instrument/ins.h"
#include "../../instrument/regs.h"

static VOID trace_malloc(UINT64 a0)
{
    fprintf(stderr, "malloc(%lu)\n", a0);
}

static VOID trace_free(UINT64 a0)
{
    fprintf(stderr, "free(0x%lx)\n", a0);
}

static VOID Instruction(INS ins, VOID* v)
{
    /* trace malloc & free */
    /* malloc.c:
     * malloc 0x1200146c8
     * free 0x120014d78
     */
    uint64_t malloc_pc = 0x1200147f0;
    uint64_t free_pc = 0x120014ea0;
    if (ins->pc == malloc_pc) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)trace_malloc, IARG_REG_VALUE, reg_a0, IARG_END);
    }
    if (ins->pc == free_pc) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)trace_free, IARG_REG_VALUE, reg_a0, IARG_END);
    }
}



#include "disas/disas.h"
static VOID print_bbl_symbol(UINT64 pc)
{
    /* FIXME: 强制让qemu调用了load_symbols（即使没开-d exec）获取符号信息，但是发现只支持静态链接的程序  */
    const char * symbol = lookup_symbol(pc);
    if (symbol && symbol[0] != '\0') {
        fprintf(stderr, "[BBL] pc: 0x%lx, symbol: %s\n", pc, symbol);
    }
}

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        /* print bbl symbol */
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)print_bbl_symbol, IARG_INST_PTR, IARG_END);
    }
}



static INT32 Usage(void)
{
    return -1;
}
 
static VOID Init(VOID)
{
}

int ins_instru(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    Init();
 
    INS_AddInstrumentFunction(Instruction, 0);
    /* show bbl's symbol */
    /* TRACE_AddInstrumentFunction(Trace, 0); */

    return 0;
}
