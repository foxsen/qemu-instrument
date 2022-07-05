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

void pin_instrument_syscall(CPUState *cpu)
{
    /* FIXME: some args are temporarily filled with NULL and 0 */
    CONTEXT ctxt;
    ctxt.env = cpu->env_ptr;
    if (PIN_state.syscall_entry_cb != NULL) {
        PIN_state.syscall_entry_cb(PIN_ThreadId(), &ctxt, 0, PIN_state.syscall_entry_cb_val);
    }
}

void pin_instrument_syscall_ret(CPUState *cpu, int num, abi_long ret)
{
    /* FIXME: some args are temporarily filled with NULL and 0 */
    CONTEXT ctxt;
    ctxt.env = cpu->env_ptr;
    if (PIN_state.syscall_exit_cb != NULL) {
        PIN_state.syscall_exit_cb(PIN_ThreadId(), &ctxt, 0, PIN_state.syscall_exit_cb_val);
    }
}
