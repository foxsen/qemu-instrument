#ifndef _LA_DISASM_H_
#define _LA_DISASM_H_

#include "ir2.h"

void la_disasm_print(uint32_t insn, char *disasm_message);
IR2_INS_OP get_ins_op(uint32_t opcode);
void la_disasm_one_ins(uint32_t opcode, IR2 *ir2);

#endif
