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

ANALYSIS_CALL *create_analysis_call(AFUNPTR funptr, int num, uint64_t args[8])
{
    ANALYSIS_CALL *call = malloc(sizeof(struct ANALYSIS_CALL));
    call->funptr = funptr;
    call->arg_nr = num;
    for (int i = 0; i < num; ++i) {
        call->args[i] = args[i];
    }
    return call;
}
