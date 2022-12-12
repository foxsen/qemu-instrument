#ifndef PIN_STATE_H
#define PIN_STATE_H

#include "ins_instrumentation.h"
#include <stdlib.h>

struct PIN_STATE {
    /* TODO: wrap them into struct, make a list, then we can registers many ins_callbacks */
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
    
    CPU_EXEC_ENTER_CALLBACK cpu_exec_enter_cb;
    VOID* cpu_exec_enter_cb_val;
    CPU_EXEC_EXIT_CALLBACK cpu_exec_exit_cb;
    VOID* cpu_exec_exit_cb_val;

    BOOL read_symbol;
    IMAGECALLBACK img_cb;
    VOID* img_cb_val;
};

struct PIN_INSTRU_CONTEXT {
    ANALYSIS_CALL ins_if_call_cb;
    bool ins_if_call_valid;
    ANALYSIS_CALL bbl_if_call_cb;
    bool bbl_if_call_valid;
};

/* FIXME 线程安全吗？ */
extern struct PIN_STATE PIN_state;
extern struct PIN_INSTRU_CONTEXT PIN_instru_ctx;

/* === 以下接口只在 QEMU 内部调用 === */
void INS_instrument(INS ins);
void TRACE_instrument(TRACE trace);
void IMG_instrument(IMG img);

/* load pintool */
void qemu_pintool_opt_parse(const char *optarg, Error **errp);

#include "../instrument/env.h"
void pin_instrument_exit(CPUArchState *env, int code);
void pin_instrument_syscall(CPUState *cpu);
void pin_instrument_syscall_ret(CPUState *cpu, int num, abi_long ret);
void pin_instrument_cpu_exec_enter(CPUState *cpu, TranslationBlock *tb);
void pin_instrument_cpu_exec_exit(CPUState *cpu, TranslationBlock *last_tb, int tb_exit);
#endif
