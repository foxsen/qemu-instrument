#ifndef _LMJ_EXAMPLE_H_
#define _LMJ_EXAMPLE_H_
#include "pin_state.h"
VOID docount(UINT64 pc, UINT32 opcode);
VOID gpr_frequency(UINT64 a, UINT64 b, UINT64 c, UINT64 d);

VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
VOID syscall_exit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v);
VOID Instruction(INS ins, VOID* v);
VOID Fini(INT32 code, VOID* v);
INT32 Usage(void);
int ins_instru(int argc, char* argv[]);

VOID docount2(UINT32 c);
VOID Trace(TRACE trace, VOID* v);
#endif
