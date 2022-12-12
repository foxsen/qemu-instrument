#ifndef INS_INSPECTION
#define INS_INSPECTION

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


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

INS INS_Next(INS x);
INS INS_Prev(INS x);
BOOL INS_Valid(INS x);
ADDRINT INS_Address (INS ins);
USIZE INS_Size (INS ins);

const char* syscall_name(uint64_t nr);

enum PREDICATE {
    PREDICATE_ALWAYS_TRUE,
    PREDICATE_INVALID,
    PREDICATE_EQUAL,
    PREDICATE_NOT_EQUAL,
    PREDICATE_LESS,
    PREDICATE_GRATER_OR_EQUAL,
    PREDICATE_LESS_UNSIGNED,
    PREDICATE_GRATER_OR_EQUAL_UNSIGNED,
    PREDICATE_ZERO,
    PREDICATE_NOT_ZERO,
    PREDICATE_LAST
};

enum MEMORY_TYPE {
    MEMORY_TYPE_READ,
    MEMORY_TYPE_WRITE,
};

/* UINT32 INS_EffectiveAddressWidth (INS ins); */
enum PREDICATE INS_GetPredicate (INS ins);
BOOL INS_IsMemoryRead (INS ins);
BOOL INS_IsMemoryWrite (INS ins);
BOOL INS_HasFallThrough (INS ins);
const char *INS_Mnemonic (INS ins);
BOOL INS_IsBranch (INS ins);
BOOL INS_IsDirectBranch (INS ins);
BOOL INS_IsDirectCall (INS ins);
BOOL INS_IsDirectControlFlow (INS ins);
/* BOOL INS_IsPcMaterialization (INS ins); */
BOOL INS_IsCall (INS ins);
BOOL INS_IsControlFlow (INS ins);
/* BOOL INS_IsValidForIpointAfter (INS ins); */
/* BOOL INS_IsValidForIpointTakenBranch (INS ins); */
/* BOOL INS_HasMemoryVector (INS ins); */
/* BOOL INS_IsProcedureCall (INS ins); */
BOOL INS_IsInterrupt (INS ins);
BOOL INS_IsRet (INS ins);
BOOL INS_IsPrefetch (INS ins);
/* BOOL INS_IsSub (const INS ins); */
BOOL INS_IsMov (const INS ins);
BOOL INS_IsAtomicUpdate (const INS ins);
BOOL INS_IsIndirectControlFlow (INS ins);
OPCODE INS_Opcode (INS ins);
/* BOOL INS_IsOriginal (INS ins); */
const char *INS_Disassemble (INS ins);
/* BOOL INS_ChangeReg (const INS ins, const REG old_reg, const REG new_reg, const BOOL as_read); */
UINT32 INS_OperandCount (INS ins);
/* BOOL INS_OperandIsMemory (INS ins, UINT32 n); */
UINT32 INS_MemoryOperandCount (INS ins);
USIZE INS_MemoryOperandSize (INS ins, UINT32 memoryOp);
BOOL INS_MemoryOperandIsRead (INS ins, UINT32 memopIdx);
BOOL INS_MemoryOperandIsWritten (INS ins, UINT32 memopIdx);
/* UINT32 INS_MemoryOperandIndexToOperandIndex (INS ins, UINT32 memopIdx); */
BOOL INS_OperandIsReg (INS ins, UINT32 n);
REG INS_OperandReg (INS ins, UINT32 n);
BOOL INS_OperandIsImmediate (INS ins, UINT32 n);
UINT64 INS_OperandImmediate (INS ins, UINT32 n);
/* BOOL INS_OperandIsImplicit (INS ins, UINT32 n); */
/* BOOL INS_RegIsImplicit (INS ins, REG reg); */
BOOL INS_OperandRead (INS ins, UINT32 n);
BOOL INS_OperandWritten (INS ins, UINT32 n);
BOOL INS_OperandReadOnly (INS ins, UINT32 n);
BOOL INS_OperandWrittenOnly (INS ins, UINT32 n);
BOOL INS_OperandReadAndWritten (INS ins, UINT32 n);
BOOL INS_IsSyscall (INS ins);
/* RTN INS_Rtn (INS x); */
INS INS_Invalid (void);
ADDRINT INS_DirectControlFlowTargetAddress (INS ins);
ADDRINT INS_NextAddress (INS ins);
/* BOOL INS_IsAddedForFunctionReplacement (INS ins); */
/* BOOL INS_IsValidForIarg (INS ins, IARG_TYPE argType); */


#ifdef __cplusplus
}
#endif
#endif
