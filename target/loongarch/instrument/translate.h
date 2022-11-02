#ifndef _TRANSALTE_H_
#define _TRANSALTE_H_

#include "env.h"
#include "ins.h"
#include <stdint.h>


extern uint64_t context_switch_bt_to_native;
extern uint64_t context_switch_native_to_bt_ret_0;
extern uint64_t context_switch_native_to_bt;

#include "tcg/tcg.h"
int la_gen_prologue(CPUState *cs, TCGContext *tcg_ctx);
int la_gen_epilogue(CPUState *cs, TCGContext *tcg_ctx);

int INS_translate(CPUState *cs, INS INS);
void INS_append_exit(INS INS, uint32_t index);

#endif
