#ifndef _LA_PRINT_H_
#define _LA_PRINT_H_

#include "assemble.h"
const char *gpr_name(uint32_t gpr);

void sprint_op(IR2_OPCODE op, char *msg);
void sprint_ins(IR2 *ir2, char * msg);
void sprint_disasm(uint32_t opcode, char *msg);

void print_op(IR2_OPCODE op);
void print_ins(IR2 *ir2);
void print_disasm(uint32_t opcode);

#endif
