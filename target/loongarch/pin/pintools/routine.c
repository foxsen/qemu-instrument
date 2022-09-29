#include "example.h"
#include "target/loongarch/pin/symbol.h"

#define MALLOC "malloc"
#define FREE "free"
 

static VOID Arg1Before(CHAR* name, ADDRINT size) {
    printf("%s(%lu)\n", name, size);
}
 
/* static VOID MallocAfter(ADDRINT ret) { */
/*     printf(" returns %lu\n", ret); */
/* } */
 

static VOID Image(IMG img, VOID* v)
{
    //  Find the malloc() function.
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);
 
        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, MALLOC, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        /* TODO */
        /* RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END); */
 
        RTN_Close(mallocRtn);
    }
 
    // Find the free() function.
    RTN freeRtn = RTN_FindByName(img, FREE);
    if (RTN_Valid(freeRtn))
    {
        RTN_Open(freeRtn);
        // Instrument free() to print the input argument value.
        RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)Arg1Before, IARG_ADDRINT, FREE, IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(freeRtn);
    }
}
 

#include "target/loongarch/instrument/elf/symbol.h"
static VOID Fini(INT32 code, VOID* v) {
    /* print_collected_symbols(); */
}

static INT32 Usage(void)
{
    return -1;
}
 

int ins_instru(int argc, char* argv[])
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
