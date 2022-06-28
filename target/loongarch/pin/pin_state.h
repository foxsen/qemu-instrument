#ifndef PIN_STATE_H
#define PIN_STATE_H

#include "ins_instrumentation.h"
#include <stdlib.h>

typedef struct PIN_STATE {
    /* TODO: wrap them into struct, make a list, so we can registers many ins_callback*/
    INS_INSTRUMENT_CALLBACK ins_cb;
    VOID* ins_cb_val;
    TRACE_INSTRUMENT_CALLBACK trace_cb;
    VOID* trace_cb_val;
    FINI_CALLBACK fini_cb;
    VOID* fini_cb_val;
    SYSCALL_ENTRY_CALLBACK syscall_entry_cb;
    VOID* syscall_entry_cb_val;
    SYSCALL_EXIT_CALLBACK syscall_exit_cb;
    VOID* syscall_exit_cb_val;
} PIN_STATE;

extern PIN_STATE PIN_state;

BOOL PIN_Init(INT32 argc, CHAR** argv);

void INS_instrument(INS ins);
void TRACE_instrument(TRACE trace);

#include "../instrument/env.h"
void pin_instrument_exit(CPUArchState *env, int code);
void pin_instrument_syscall(CPUState *cpu, int64_t num, uint64_t a1, uint64_t a2,
                            uint64_t a3, uint64_t a4, uint64_t a5,
                            uint64_t a6, uint64_t a7, uint64_t a8);
void pin_instrument_syscall_ret(void *cpu, int num, abi_long ret);
#endif
