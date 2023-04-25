#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
#include <assert.h>

#include <unordered_map>
#include <vector>
#include <algorithm>

#include "../../instrument/util/bitopts.h"
static UINT64 calc_offset(Ins * ins)
{
    uint64_t offset = 0;
    switch (ins->op) {
        case LISA_BEQ:
        case LISA_BNE:
        case LISA_BLT:
        case LISA_BGE:
        case LISA_BLTU:
        case LISA_BGEU:
            offset = sign_extend(ins->opnd[2].val << 2, 18);
            break;
        case LISA_BEQZ:
        case LISA_BNEZ:
            offset = sign_extend(ins->opnd[1].val << 2, 23);
            break;
        case LISA_B:
        case LISA_BL:
            offset = sign_extend(ins->opnd[0].val << 2, 28);
            break;
        case LISA_BCEQZ:
        case LISA_BCNEZ:
            offset = sign_extend(ins->opnd[1].val << 2, 23);
            break;
        default:
            assert(0);
            offset = 0;
            break;
    }
    return offset;
}


std::unordered_map<IR2_OPCODE, std::unordered_map<uint64_t, uint64_t*>> m;

static VOID init_map()
{
    m[LISA_BEQZ] = {};
    m[LISA_BNEZ] = {};
    m[LISA_BCEQZ] = {};
    m[LISA_BCNEZ] = {};
    m[LISA_B] = {};
    m[LISA_BL] = {};
    m[LISA_BEQ] = {};
    m[LISA_BNE] = {};
    m[LISA_BLT] = {};
    m[LISA_BGE] = {};
    m[LISA_BLTU] = {};
    m[LISA_BGEU] = {};
}

static VOID Instruction(INS ins, VOID* v)
{
    if (INS_IsDirectBranch(ins)) {
        IR2_OPCODE op = ins->origin_ins->op;
        UINT64 offset = calc_offset(ins->origin_ins);
        if (m[op].count(offset) == 0) {
            m[op][offset] = (uint64_t*)malloc(sizeof(uint64_t));
            *m[op][offset] = 0;
        }
        INS_InsertInlineAdd(ins, IPOINT_BEFORE, m[op][offset], 1, false);
    }
}

static VOID Init(VOID)
{
    init_map();
}

struct record {
    uint64_t addr;
    uint64_t cnt;
};

static VOID output_res(IR2_OPCODE op, const char * name)
{
    fprintf(stderr, "%s\n", name);
    std::vector<record> vec;
    const auto & mm = m[op];
    for (const auto& kv : mm) {
        vec.push_back({kv.first, *kv.second});
    }
    sort(vec.begin(), vec.end(), [](const record & a, const record & b){return a.addr < b.addr;});
    for (const record &r : vec) {
        fprintf(stderr, "0x%lx %ld\n", r.addr, r.cnt);
    }
}

static VOID Fini(INT32 code, VOID* v)
{
    output_res(LISA_BEQ, "LISA_BEQ");
    output_res(LISA_BNE, "LISA_BNE");
    output_res(LISA_BLT, "LISA_BLT");
    output_res(LISA_BGE, "LISA_BGE");
    output_res(LISA_BLTU, "LISA_BLTU");
    output_res(LISA_BGEU, "LISA_BGEU");
    output_res(LISA_BEQZ, "LISA_BEQZ");
    output_res(LISA_BNEZ, "LISA_BNEZ");
    output_res(LISA_B, "LISA_B");
    output_res(LISA_BL, "LISA_BL");
    output_res(LISA_BCEQZ, "LISA_BCEQZ");
    output_res(LISA_BCNEZ, "LISA_BCNEZ");
    fprintf(stderr, "that's all.\n");
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    Init();

    INS_AddInstrumentFunction(Instruction, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
