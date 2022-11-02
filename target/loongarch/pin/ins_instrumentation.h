#ifndef INS_TRUMENTATION_H
#define INS_TRUMENTATION_H

#include "types.h"
#include "instrumentation_arguements.h"
#include "thread.h"
#include "context.h"

typedef VOID (*INS_INSTRUMENT_CALLBACK) (INS ins, VOID* v);
typedef VOID (*TRACE_INSTRUMENT_CALLBACK) (TRACE trace, VOID *v);
typedef VOID (*FINI_CALLBACK) (INT32 code, VOID *v);
typedef VOID (*SYSCALL_ENTRY_CALLBACK) (THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
typedef VOID (*SYSCALL_EXIT_CALLBACK) (THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
typedef VOID (*CPU_EXEC_ENTER_CALLBACK) (CPUState *cpu, TranslationBlock *tb);
typedef VOID (*CPU_EXEC_EXIT_CALLBACK) (CPUState *cpu, TranslationBlock *last_tb, int tb_exit);
typedef VOID (*IMAGECALLBACK) (IMG img, VOID *v);

/* FIXME: change the return type(PIN_CALLBACK) to (void) because I do not know the class of PIN_CALLBACK */
void INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val);
void TRACE_AddInstrumentFunction(TRACE_INSTRUMENT_CALLBACK fun, VOID *val);
void PIN_AddFiniFunction(FINI_CALLBACK fun, VOID *val); 
void PIN_AddSyscallEntryFunction(SYSCALL_ENTRY_CALLBACK fun, VOID *val);
void PIN_AddSyscallExitFunction(SYSCALL_EXIT_CALLBACK fun, VOID *val);
void PIN_AddCpuExecEnterFunction(CPU_EXEC_ENTER_CALLBACK fun, VOID *val);
void PIN_AddCpuExecExitFunction(CPU_EXEC_EXIT_CALLBACK fun, VOID *val);
void IMG_AddInstrumentFunction(IMAGECALLBACK fun, VOID *val);

VOID INS_InsertCall(INS INS, IPOINT action, AFUNPTR funptr, ...);
VOID BBL_InsertCall(BBL bbl, IPOINT action, AFUNPTR funptr, ...);
VOID TRACE_InsertCall(TRACE trace, IPOINT action, AFUNPTR funptr, ...);
VOID RTN_InsertCall(RTN rtn, IPOINT action, AFUNPTR funptr, ...);
VOID RTN_instrument(TRACE trace);

/* add imm to value in ptr */
VOID INS_InsertInlineAdd(INS ins, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic);
VOID BBL_InsertInlineAdd(BBL bbl, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic);

/* TODO PIN doc: In Linux systems with kernel 5.3 and above, calling this API while jumping to vsyscall area will return special number - VSYSCALL_NR. */
ADDRINT PIN_GetSyscallNumber (const CONTEXT *ctxt, SYSCALL_STANDARD std);
ADDRINT PIN_GetSyscallArgument (const CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum);
VOID PIN_SetSyscallNumber (CONTEXT *ctxt, SYSCALL_STANDARD std, ADDRINT val);
VOID PIN_SetSyscallArgument (CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum, ADDRINT val);


#endif
