#include "la_disasm/include/disasm.h"
#include "la_disasm/include/assemble.h"
#include "debug.h"

void INS_dump(INS ins)
{
    char ins_info[128];
    la_disasm_print(ins->opcode, ins_info);
    fprintf(stderr, "0x%-16lx: %08x\t%s\n", ins->pc, ins->opcode, ins_info);
    fprintf(stderr, "------------------\n");

    Ins *inst = ins->first_ins;
    while (inst) {
        uint32_t opcode = la_assemble(inst);
        la_disasm_print(opcode, ins_info);
        fprintf(stderr, "%08x\t%s\n", opcode, ins_info);

        if (inst == ins->last_ins)
            break;
        inst = inst->next;
    }
    fprintf(stderr, "------------------\n");
}
