#ifndef PIN_CONTEXT_H
#define PIN_CONTEXT_H

#include "../instrument/env.h"
/* 另一种include方案： */
/* #include "qemu/osdep.h" */
/* #include "exec/exec-all.h" */
/* #include "target/loongarch/cpu_loongarch_state.h" */

typedef struct CONTEXT {
    CPULoongArchState *env;
} CONTEXT;

/* #include "../instrument/regs.h" */

#endif
