#ifndef PIN_STATE_H
#define PIN_STATE_H

#include "ins_instrumentation.h"
#include <stdlib.h>

typedef struct PIN_STATE {
    /* TODO: wrap them into struct, make a list, so we can registers many ins_callback*/
    INS_INSTRUMENT_CALLBACK ins_cb;
    VOID* ins_cb_val;
    FINI_CALLBACK fini_cb;
    VOID* fini_cb_val;
} PIN_STATE;

extern PIN_STATE PIN_state;

BOOL PIN_Init(INT32 argc, CHAR** argv);

typedef struct ANALYSIS_CALL {
    AFUNPTR funptr;
    int arg_nr;
    uint64_t args[8];
} ANALYSIS_CALL;

void INS_instrument(INS ins);
ANALYSIS_CALL *create_analysis_call(AFUNPTR funptr, int num, uint64_t args[8]);
#endif
