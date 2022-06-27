#ifndef _LMJ_EXAMPLE_H_
#define _LMJ_EXAMPLE_H_
#include "pin_state.h"
VOID docount(UINT64 pc, UINT32 opcode);
VOID Instruction(INS ins, VOID* v);
VOID Fini(INT32 code, VOID* v);
INT32 Usage(void);
int ins_instru(int argc, char* argv[]);

VOID docount2(UINT32 c);
VOID Trace(TRACE trace, VOID* v);
#endif
