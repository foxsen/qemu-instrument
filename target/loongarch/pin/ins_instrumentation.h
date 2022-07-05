#ifndef INS_TRUMENTATION_H
#define INS_TRUMENTATION_H

#include "types.h"
#include "instrumentation_arguements.h"
#include "thread.h"
#include "context.h"

typedef VOID (*INS_INSTRUMENT_CALLBACK) (INS ins, VOID* v);
typedef VOID (*TRACE_INSTRUMENT_CALLBACK) (TRACE trace, VOID *v);
typedef VOID (* FINI_CALLBACK) (INT32 code, VOID *v);
typedef VOID (* SYSCALL_ENTRY_CALLBACK) (THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
typedef VOID (* SYSCALL_EXIT_CALLBACK) (THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);

/* FIXME: change the return type(PIN_CALLBACK) to (void) because I dont know the class of PIN_CALLBACK */
void INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val);
void TRACE_AddInstrumentFunction(TRACE_INSTRUMENT_CALLBACK fun, VOID *val);
void PIN_AddFiniFunction(FINI_CALLBACK fun, VOID *val); 
void PIN_AddSyscallEntryFunction(SYSCALL_ENTRY_CALLBACK fun, VOID *val);
void PIN_AddSyscallExitFunction(SYSCALL_EXIT_CALLBACK fun, VOID *val);

VOID INS_InsertCall(INS ins, IPOINT action, AFUNPTR funptr, ...);
VOID BBL_InsertCall(BBL bbl, IPOINT action, AFUNPTR funptr, ...);
VOID TRACE_InsertCall(TRACE trace, IPOINT action, AFUNPTR funptr, ...);

#endif
