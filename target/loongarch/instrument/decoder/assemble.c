#include "assemble.h"
#include "../util/error.h"

uint32_t la_assemble(Ins *ins)
{
    GM_LA_OPCODE_FORMAT format = lisa_format_table[ins->op];
    lsassert(format.op == ins->op);
    lsassert(format.opcode != 0);

    uint32_t insn = format.opcode;
    lsassertm(insn, "Cannot use a pseudo opcode!");

    for (int i = 0; i < 4; ++i) {
        GM_OPERAND_TYPE opnd_type = format.opnd[i];
        if (opnd_type == OPD_INVALID) {
            lsassert(i == ins->opnd_count);
            break;
        }

        int val = ins->opnd[i].val;

        GM_OPERAND_PLACE_RELATION bit_field = bit_field_table[opnd_type];
        int start = bit_field.bit_range_0.start;
        int end = bit_field.bit_range_0.end;
        int bit_len = end - start + 1;

        int mask = (1 << bit_len) - 1;
        insn |= (val & mask) << start;

        /* OPD_OFFL & OPD_OFFLL opnd has two parts */
        start = bit_field.bit_range_1.start;
        end = bit_field.bit_range_1.end;
        if (start >= 0 && end >= 0) {
            val = val >> bit_len;
            bit_len = end - start + 1;
            mask = (1 << bit_len) - 1;
            insn |= (val & mask) << start;
        }
    }
    return insn;
}

