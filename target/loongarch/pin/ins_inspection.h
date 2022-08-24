#ifndef INS_INSPECTION
#define INS_INSPECTION

#include "types.h"

ADDRINT INS_Address(INS ins);
BOOL INS_IsMemoryRead(INS ins);
BOOL INS_IsMemoryWrite(INS ins);
USIZE INS_MemoryOperandSize(INS ins, UINT32 memoryOp);
BOOL INS_HasFallThrough(INS ins);

BBL TRACE_BblHead(TRACE trace);
BBL TRACE_BblTail(TRACE trace);
UINT32 TRACE_NumBbl(TRACE trace);
UINT32 TRACE_NumIns(TRACE trace);

UINT32 BBL_NumIns(BBL bbl);
INS BBL_InsHead(BBL x);
INS BBL_InsTail(BBL x);
BBL BBL_Next(BBL x);
BBL BBL_Prev(BBL x);
BOOL BBL_Valid(BBL x);

const char* syscall_name(uint64_t nr);
#endif
