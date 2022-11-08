#include "pintool.h"
#include "../ins_inspection.h"
#include "../symbol.h"

#include <unordered_map>
#include <vector>
#include <algorithm>

struct bbl_info {
    bbl_info(uint64_t pc) : pc(pc), cnt(0) {}
    uint64_t pc;
    uint64_t cnt;
};

std::unordered_map<uint64_t, bbl_info *> m;
std::vector<bbl_info *> vec;

static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        uint64_t pc = bbl->pc;
        if (m.count(pc) == 0) {
            bbl_info *p = new bbl_info(pc);
            vec.push_back(p);
            m[pc] = p;
        }
        BBL_InsertInlineAdd(bbl, IPOINT_BEFORE, &(m[pc]->cnt), 1, false);
    }
}

static VOID Fini(INT32 code, VOID* v)
{
    std::sort(vec.begin(), vec.end(), [](bbl_info *a, bbl_info * b) { return a->cnt > b->cnt; });
    uint64_t sum = 0;
    for (bbl_info * info : vec) {
        sum += info->cnt;
    }
    fprintf(stderr, "\nHot block Top 20 (of total %lu blocks):\n", vec.size());
    for (int i = 0; i < 20; ++i) {
        fprintf(stderr, "%2d  0x%08lx\t%lu\t%5.2f%%    %s\n", i, vec[i]->pc, vec[i]->cnt, (double)vec[i]->cnt/sum*100, RTN_FindNameByAddress(vec[i]->pc));
    }
    fprintf(stderr, "Total block execute count: %lu\n", sum);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int pintool_install(int argc, char* argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
