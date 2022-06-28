#include "la_disasm/include/disasm.h"
#include "la_disasm/include/assemble.h"
#include "debug.h"

void Ins_dump(Ins *ins)
{
    char msg[128];
    uint32_t opcode = la_assemble(ins);
    la_disasm_print(opcode, msg);
    fprintf(stderr, "%08x\t%s\n", opcode, msg);
}

void INS_dump(INS ins)
{
    char msg[128];
    la_disasm_print(ins->opcode, msg);
    fprintf(stderr, "0x%-16lx: %08x\t%s\n", ins->pc, ins->opcode, msg);
    fprintf(stderr, "------------------\n");

    Ins *inst = ins->first_ins;
    while (inst) {
        uint32_t opcode = la_assemble(inst);
        la_disasm_print(opcode, msg);
        fprintf(stderr, "%08x\t%s\n", opcode, msg);

        if (inst == ins->last_ins)
            break;
        inst = inst->next;
    }
    fprintf(stderr, "------------------\n");
}
