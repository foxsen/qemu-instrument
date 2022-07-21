#ifndef _LA_DISASM_H_
#define _LA_DISASM_H_

#include "ir2.h"
#include "assemble.h"

IR2_OPCODE get_ins_op(uint32_t insn);
int get_opnd_val(GM_OPERAND_TYPE type, uint32_t insn);
void la_disasm(uint32_t opcode, Ins *ins);

#endif
