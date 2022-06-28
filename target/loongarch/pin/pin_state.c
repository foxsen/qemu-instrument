#include "pin_state.h"

PIN_STATE PIN_state;

BOOL PIN_Init(INT32 argc, CHAR** argv)
{
    PIN_state.ins_cb = NULL;
    PIN_state.ins_cb_val = NULL;
    return 0;
}

void INS_instrument(INS ins)
{
    if (PIN_state.ins_cb != NULL) {
        PIN_state.ins_cb(ins, PIN_state.ins_cb_val);
    }
}

void TRACE_instrument(TRACE trace)
{
    if (PIN_state.trace_cb != NULL) {
        PIN_state.trace_cb(trace, PIN_state.trace_cb_val);
    }
}

void pin_instrument_exit(CPUArchState *env, int code)
{
    if (PIN_state.fini_cb != NULL) {
        PIN_state.fini_cb(code, PIN_state.fini_cb_val);
    }
}

void pin_instrument_syscall(CPUState *cpu, int64_t num, uint64_t a1, uint64_t a2,
                              uint64_t a3, uint64_t a4, uint64_t a5,
                              uint64_t a6, uint64_t a7, uint64_t a8)
{
    /* todo: make args and call PIN_state... */
    if (PIN_state.syscall_entry_cb != NULL) {
        PIN_state.syscall_entry_cb(..., PIN_state.syscall_entry_cb_val);
    }
}

void pin_instrument_syscall_ret(void *cpu, int num, abi_long ret)
{
    /* todo: make args and call PIN_state.... */
    if (PIN_state.syscall_exit_cb != NULL) {
        PIN_state.syscall_exit_cb(..., PIN_state.syscall_exit_cb_val);
    }
}
