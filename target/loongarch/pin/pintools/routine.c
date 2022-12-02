#include "pintool.h"
#include "../symbol.h"
#include "../../instrument/elf/symbol.h"

#define MALLOC "malloc"
#define FREE "free"
#define FOO "foo"

static VOID Arg1Before(CHAR* name, ADDRINT size) {
    fprintf(stderr, "%s(%lx)\n", name, size);
}
 
static VOID MallocAfter(ADDRINT ret) {
    fprintf(stderr, " returns %lx\n", ret);
}

static VOID Image(IMG img, VOID* v)
{
    /* Malloc */
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, MALLOC, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
        RTN_Close(mallocRtn);
    }
 
    /* Free */
    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, FREE, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_Close(freeRtn);
    }

    /* Foo */
    RTN fooRtn = RTN_FindByName(img, FOO);
    if (RTN_Valid(fooRtn))
    {
        RTN_Open(fooRtn);
        RTN_InsertCall(fooRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, FOO, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
        RTN_InsertCall(fooRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
        RTN_Close(fooRtn);
    }
}
 

static VOID Fini(INT32 code, VOID* v) {
    /* print_collected_symbols(); */
}

static INT32 Usage(void)
{
    return -1;
}
 

int main(int argc, char* argv[])
{
    // Initialize pin & symbol manager
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();
 
    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);
 
    /* PIN_StartProgram(); */
 
    return 0;
}
