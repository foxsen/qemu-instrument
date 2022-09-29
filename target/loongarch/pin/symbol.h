#ifndef PIN_SYMBOL_H
#define PIN_SYMBOL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
    RTN RTN_FindByName(IMG img, const CHAR *name);
    BOOL RTN_Valid(RTN x);
    VOID RTN_Open(RTN rtn);
    VOID RTN_Close(RTN rtn);
#ifdef __cplusplus
}
#endif

#include "instrumentation_arguements.h"
#ifdef __cplusplus
extern "C" {
#endif
    VOID RTN_save_cb(uintptr_t addr, ANALYSIS_CALL cb);
    ANALYSIS_CALL *RTN_get_cbs(uintptr_t addr, int *n);
#ifdef __cplusplus
}
#endif

#endif
