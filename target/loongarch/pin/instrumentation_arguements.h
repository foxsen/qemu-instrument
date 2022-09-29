#ifndef INSTRUMENTATION_ARGUMENTS_H
#define INSTRUMENTATION_ARGUMENTS_H

/*! @ingroup INST_ARGS
 *
 * Determines where the analysis call is inserted relative to the instrumented
 * object. Instrumented object can be: INS, BBL, TRACE, RTN. This is used in
 * instrumentation functions, for example see @ref TRACE_InsertCall().
 */
typedef enum {
  IPOINT_INVALID,
  IPOINT_BEFORE,
  IPOINT_AFTER,
  IPOINT_ANYWHERE,
  IPOINT_TAKEN_BRANCH
} IPOINT;

typedef enum {
  CALL_ORDER_FIRST = 100,
  CALL_ORDER_DEFAULT = 200,
  CALL_ORDER_LAST = 300
} CALL_ORDER;

typedef enum {
  IARG_INVALID,
  IARG_ADDRINT,
  IARG_PTR,
  IARG_BOOL,
  IARG_UINT32,
  IARG_UINT64,
  IARG_INST_PTR,
  IARG_REG_VALUE,
  IARG_REG_REFERENCE,
  IARG_REG_CONST_REFERENCE,
  IARG_MEMORYREAD_EA,
  IARG_MEMORYREAD2_EA,
  IARG_MEMORYWRITE_EA,
  IARG_MEMORYREAD_SIZE,
  IARG_MEMORYWRITE_SIZE,
  IARG_MEMORYREAD_PTR,
  IARG_MEMORYREAD2_PTR,
  IARG_MEMORYWRITE_PTR,
  IARG_MEMORYOP_PTR,
  IARG_MULTI_MEMORYACCESS_EA,
  IARG_MULTI_ELEMENT_OPERAND,
  IARG_REWRITE_SCATTERED_MEMOP,
  IARG_EXPLICIT_MEMORY_EA,
  IARG_BRANCH_TAKEN,
  IARG_BRANCH_TARGET_ADDR,
  IARG_FALLTHROUGH_ADDR,
  IARG_EXECUTING,
  IARG_FIRST_REP_ITERATION,
  IARG_PREDICATE,
  IARG_STACK_VALUE,
  IARG_STACK_REFERENCE,
  IARG_MEMORY_VALUE,
  IARG_MEMORY_REFERENCE,
  IARG_SYSCALL_NUMBER,
  IARG_SYSARG_REFERENCE,
  IARG_SYSARG_VALUE,
  IARG_SYSRET_VALUE,
  IARG_SYSRET_ERRNO,
  IARG_FUNCARG_CALLSITE_REFERENCE,
  IARG_FUNCARG_CALLSITE_VALUE,
  IARG_FUNCARG_ENTRYPOINT_REFERENCE,
  IARG_FUNCARG_ENTRYPOINT_VALUE,
  IARG_FUNCRET_EXITPOINT_REFERENCE,
  IARG_FUNCRET_EXITPOINT_VALUE,
  IARG_RETURN_IP,
  IARG_ORIG_FUNCPTR,
  IARG_PROTOTYPE,
  IARG_THREAD_ID,
  IARG_CONTEXT,
  IARG_CONST_CONTEXT,
  IARG_PARTIAL_CONTEXT,
  IARG_PRESERVE,
  IARG_RETURN_REGS,
  IARG_CALL_ORDER,
  IARG_IARGLIST,
  IARG_FAST_ANALYSIS_CALL,
  IARG_MEMORYOP_EA,
  IARG_MEMORYOP_SIZE,
  IARG_MEMORYOP_MASKED_ON,
  IARG_TSC,
  IARG_FILE_NAME,
  IARG_LINE_NO,
  IARG_LAST,
  IARG_END
} IARG_TYPE;

typedef enum { PIN_MEMOP_LOAD, PIN_MEMOP_STORE } PIN_MEMOP_ENUM;

typedef enum {
  PIN_OP_ELEMENT_ACCESS_READ,
  PIN_OP_ELEMENT_ACCESS_WRITE,
  PIN_OP_ELEMENT_ACCESS_READWRITE
} PIN_OP_ELEMENT_ACCESS;


/* 下面的不是Pin定义的类型 */
#include "types.h"
typedef enum {
    IOBJECT_INS,
    IOBJECT_BBL,
    IOBJECT_RTN,
} IOBJECT;

typedef struct analysis_call_arg {
    IARG_TYPE type;
    uint64_t val;
} IARG_T;

typedef struct analysis_call {
    AFUNPTR func;
    IOBJECT object;
    IPOINT action;
    int arg_cnt;
    IARG_T arg[8];
} ANALYSIS_CALL;
#endif
