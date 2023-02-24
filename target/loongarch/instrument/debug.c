#include "debug.h"
#include "decoder/assemble.h"
#include "decoder/la_print.h"

void Ins_dump(Ins *ins)
{
    char msg[128];
    uint32_t opcode = la_assemble(ins);
    sprint_ins(ins, msg);
    fprintf(stderr, "%08x\t%s\n", opcode, msg);
}

void INS_dump(INS INS)
{
    char msg[128];
    sprint_ins(INS->origin_ins, msg);
    fprintf(stderr, "0x%-16lx: %08x\t%s\n", INS->pc, INS->opcode, msg);
    fprintf(stderr, "..................\n");

    Ins *ins = INS->first_ins;
    while (ins) {
        uint32_t opcode = la_assemble(ins);
        sprint_ins(ins, msg);
        fprintf(stderr, "%08x\t%s\n", opcode, msg);

        if (ins == INS->last_ins)
            break;
        ins = ins->next;
    }
    fprintf(stderr, "------------------\n");
}
