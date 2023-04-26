#ifndef PIN_SYMBOL_H
#define PIN_SYMBOL_H

#include "types.h"
#include "instrumentation_arguements.h"

#ifdef __cplusplus
extern "C" {
#endif
    /* In Pin, RTN_FindNameByAddress return std::string */
    const CHAR *RTN_FindNameByAddress(ADDRINT address);
    RTN RTN_FindByName(IMG img, const CHAR *name);
    BOOL RTN_Valid(RTN x);
    VOID RTN_Open(RTN rtn);
    VOID RTN_Close(RTN rtn);

    /* === 下面为内部实现所需接口 === */
    VOID RTN_add_entry_cb(RTN rtn, ANALYSIS_CALL *cb);
    ANALYSIS_CALL *RTN_get_entry_cbs(uintptr_t pc, int *cnt);
    VOID RTN_add_exit_cb(RTN rtn, ANALYSIS_CALL *cb);
    ANALYSIS_CALL *RTN_get_exit_cbs(uintptr_t pc, int *cnt);
#ifdef __cplusplus
}
#endif

#endif
