#include "pintool.h"
/* #include "../ins_inspection.h" */
#include <stdio.h>
#include <stdlib.h>

FILE *trace;

const INT32 N = 2;

INT32 icount = N;

INT32 ins_cnt = 0;
VOID ICount()
{
    ++ins_cnt;
    printf("ins_cnt: %d\n", ins_cnt);
}

ADDRINT CountDown()
{
    --icount;
    return (icount == 0);
}

// The IP of the current instruction will be printed and
// the icount will be reset to a random number between N and N+M.
VOID PrintIp(VOID* ip)
{
    printf("than called , ip=%p\n", ip);
    fprintf(trace, "%p\n", ip);

    // Prepare for next period
    icount = N;
}

VOID Instruction(INS ins, VOID* v)
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ICount, IARG_END);

    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)CountDown, IARG_END);

    INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)PrintIp, IARG_INST_PTR, IARG_END);
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID* v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
}

INT32 Usage()
{
    return -1;
}

int main(int argc, char* argv[])
{
    trace = fopen("isampling.out", "w");

    if (PIN_Init(argc, argv)) return Usage();

    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddFiniFunction(Fini, 0);


    return 0;
}

