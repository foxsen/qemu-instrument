#ifndef _LA_DISASM_H_
#define _LA_DISASM_H_

#include "ins.h"

#ifdef __cplusplus
extern "C" {
#endif

void la_disasm(uint32_t opcode, Ins *ins);
LA_OPCODE get_ins_op(uint32_t insn);

#ifdef __cplusplus
}
#endif
#endif
