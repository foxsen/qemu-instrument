#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_


#include "qemu/typedefs.h"
/* #include "exec/translator.h" */
int la_decode(CPUState *cs, TranslationBlock *tb, int max_insns);
void la_relocation(CPUState *cs, const void *code_buf_rx);

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/qemu-print.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "sysemu/qtest.h"
#include "exec/exec-all.h"
#include "qapi/qapi-commands-machine-target.h"
/* FIXME: include above 后可以 include cpu.h，也许可以更少 */
#include "tcg/tcg.h"
int la_encode(TCGContext *tcg_ctx, void *code_buf);

#endif
