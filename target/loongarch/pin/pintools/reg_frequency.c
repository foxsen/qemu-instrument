#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../../instrument/decoder/ins.h"
#include "../ins_inspection.h"
#include "../../instrument/regs.h"

UINT64 gpr_cnt[32] = {0};

static VOID gpr_frequency1(UINT64 a, UINT64 b, UINT64 c, UINT64 d, UINT64 e, UINT64 f, UINT64 g, UINT64 h)
{
    gpr_cnt[0] += a >> 32;  gpr_cnt[1] += a & 0xffffffff;
    gpr_cnt[2] += b >> 32;  gpr_cnt[3] += b & 0xffffffff;
    gpr_cnt[4] += c >> 32;  gpr_cnt[5] += c & 0xffffffff;
    gpr_cnt[6] += d >> 32;  gpr_cnt[7] += d & 0xffffffff;
    gpr_cnt[8] += e >> 32;  gpr_cnt[9] += e & 0xffffffff;
    gpr_cnt[10] += f >> 32;  gpr_cnt[11] += f & 0xffffffff;
    gpr_cnt[12] += g >> 32;  gpr_cnt[13] += g & 0xffffffff;
    gpr_cnt[14] += h >> 32;  gpr_cnt[15] += h & 0xffffffff;
    /* gpr_cnt[0] += a >> 48;  gpr_cnt[1] += (a >> 32) & 0xffff;   gpr_cnt[2] += (a >> 16) & 0xffff;   gpr_cnt[3] += a & 0xffff; */
    /* gpr_cnt[4] += b >> 48;  gpr_cnt[5] += (b >> 32) & 0xffff;   gpr_cnt[6] += (b >> 16) & 0xffff;   gpr_cnt[7] += b & 0xffff; */
    /* gpr_cnt[8] += c >> 48;  gpr_cnt[9] += (c >> 32) & 0xffff;   gpr_cnt[10] += (c >> 16) & 0xffff;   gpr_cnt[11] += c & 0xffff; */
    /* gpr_cnt[12] += d >> 48;  gpr_cnt[13] += (d >> 32) & 0xffff;   gpr_cnt[14] += (d >> 16) & 0xffff;   gpr_cnt[15] += d & 0xffff; */
    /* gpr_cnt[16] += e >> 48;  gpr_cnt[17] += (e >> 32) & 0xffff;   gpr_cnt[18] += (e >> 16) & 0xffff;   gpr_cnt[19] += e & 0xffff; */
    /* gpr_cnt[20] += f >> 48;  gpr_cnt[21] += (f >> 32) & 0xffff;   gpr_cnt[22] += (f >> 16) & 0xffff;   gpr_cnt[23] += f & 0xffff; */
    /* gpr_cnt[24] += g >> 48;  gpr_cnt[25] += (g >> 32) & 0xffff;   gpr_cnt[26] += (g >> 16) & 0xffff;   gpr_cnt[27] += g & 0xffff; */
    /* gpr_cnt[28] += h >> 48;  gpr_cnt[29] += (h >> 32) & 0xffff;   gpr_cnt[30] += (h >> 16) & 0xffff;   gpr_cnt[31] += h & 0xffff; */
}

static VOID gpr_frequency2(UINT64 a, UINT64 b, UINT64 c, UINT64 d, UINT64 e, UINT64 f, UINT64 g, UINT64 h)
{
    gpr_cnt[16] += a >> 32;  gpr_cnt[17] += a & 0xffffffff;
    gpr_cnt[18] += b >> 32;  gpr_cnt[19] += b & 0xffffffff;
    gpr_cnt[20] += c >> 32;  gpr_cnt[21] += c & 0xffffffff;
    gpr_cnt[22] += d >> 32;  gpr_cnt[23] += d & 0xffffffff;
    gpr_cnt[24] += e >> 32;  gpr_cnt[25] += e & 0xffffffff;
    gpr_cnt[26] += f >> 32;  gpr_cnt[27] += f & 0xffffffff;
    gpr_cnt[28] += g >> 32;  gpr_cnt[29] += g & 0xffffffff;
    gpr_cnt[30] += h >> 32;  gpr_cnt[31] += h & 0xffffffff;
}




static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        /* Statistic GPR Access Frequency */
        uint64_t cnt[32] = {0};
        for (INS ins = bbl->ins_head; ins != NULL; ins = ins->next) {
            for (int i = 0; i < 4; ++i) {
                if (opnd_is_gpr(ins->origin_ins, i)) {
                    ++cnt[ins->origin_ins->opnd[i].val];
                }
            }
        }
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)gpr_frequency1,
                IARG_UINT64, (cnt[0] << 32) + cnt[1], 
                IARG_UINT64, (cnt[2] << 32) + cnt[3], 
                IARG_UINT64, (cnt[4] << 32) + cnt[5], 
                IARG_UINT64, (cnt[6] << 32) + cnt[7], 
                IARG_UINT64, (cnt[8] << 32) + cnt[9], 
                IARG_UINT64, (cnt[10] << 32) + cnt[11], 
                IARG_UINT64, (cnt[12] << 32) + cnt[13], 
                IARG_UINT64, (cnt[14] << 32) + cnt[15], 
                IARG_END);
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)gpr_frequency2,
                IARG_UINT64, (cnt[16] << 32) + cnt[17], 
                IARG_UINT64, (cnt[18] << 32) + cnt[19], 
                IARG_UINT64, (cnt[20] << 32) + cnt[21], 
                IARG_UINT64, (cnt[22] << 32) + cnt[23], 
                IARG_UINT64, (cnt[24] << 32) + cnt[25], 
                IARG_UINT64, (cnt[26] << 32) + cnt[27], 
                IARG_UINT64, (cnt[28] << 32) + cnt[29], 
                IARG_UINT64, (cnt[30] << 32) + cnt[31], 
                IARG_END);
    }
}


static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "GPR Frequency Count:\n");
    for (int i = 0; i < 32; ++i) {
        fprintf(stderr, "r%d: %lu\n", i, gpr_cnt[i]);
    }
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
