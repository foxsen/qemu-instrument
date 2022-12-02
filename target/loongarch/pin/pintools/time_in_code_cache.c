#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"

static uint64_t bbl_exec_nr = 0;
static uint64_t ins_exec_nr = 0;
static VOID bbl_statistic(uint64_t ins_nr) {
    ++bbl_exec_nr;
    ins_exec_nr += ins_nr;
}

static uint64_t total_trans_ins_nr = 0;
static VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        /* BBL count */
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)bbl_statistic, IARG_UINT64, bbl->nr_ins, IARG_END);
        total_trans_ins_nr += bbl->nr_ins;
    }
}

#include <time.h>

static UINT64 cpu_enter_cnt = 0;
static UINT64 cpu_exit_cnt = 0;
static UINT64 syscall_exit_cnt = 0;
static struct timespec tv_enter = {0, 0};
static struct timespec tv_exit = {0, 0};
static struct timespec tv_qemu = {0, 0};
static struct timespec tv_code = {0, 0};

static VOID Init(VOID)
{
    clock_gettime(CLOCK_MONOTONIC, &tv_exit);
}

static VOID cpu_exec_enter(CPUState *cpu, TranslationBlock *tb)
{
    clock_gettime(CLOCK_MONOTONIC, &tv_enter);
    tv_qemu.tv_sec = tv_enter.tv_sec - tv_exit.tv_sec + tv_qemu.tv_sec;
    tv_qemu.tv_nsec = tv_enter.tv_nsec - tv_exit.tv_nsec + tv_qemu.tv_nsec;
    if (tv_qemu.tv_nsec >= 1000000000) {
        ++tv_qemu.tv_sec;
        tv_qemu.tv_nsec -= 1000000000;
    }
    if (tv_qemu.tv_nsec < 0) {
        --tv_qemu.tv_sec;
        tv_qemu.tv_nsec += 1000000000;
    }

    ++cpu_enter_cnt;
}

static VOID cpu_exec_exit(CPUState *cpu, TranslationBlock *last_tb, int tb_exit)
{
    clock_gettime(CLOCK_MONOTONIC, &tv_exit);
    tv_code.tv_sec = tv_exit.tv_sec - tv_enter.tv_sec + tv_code.tv_sec;
    tv_code.tv_nsec = tv_exit.tv_nsec - tv_enter.tv_nsec + tv_code.tv_nsec;
    if (tv_code.tv_nsec >= 1000000000) {
        ++tv_code.tv_sec;
        tv_code.tv_nsec -= 1000000000;
    }
    if (tv_code.tv_nsec < 0) {
        --tv_code.tv_sec;
        tv_code.tv_nsec += 1000000000;
    }

    ++cpu_exit_cnt;
}

static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    clock_gettime(CLOCK_MONOTONIC, &tv_exit);
    tv_code.tv_sec = tv_exit.tv_sec - tv_enter.tv_sec + tv_code.tv_sec;
    tv_code.tv_nsec = tv_exit.tv_nsec - tv_enter.tv_nsec + tv_code.tv_nsec;
    if (tv_code.tv_nsec >= 1000000000) {
        ++tv_code.tv_sec;
        tv_code.tv_nsec -= 1000000000;
    }
    if (tv_code.tv_nsec < 0) {
        --tv_code.tv_sec;
        tv_code.tv_nsec += 1000000000;
    }

    ++syscall_exit_cnt;
}

static VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "CPU Enter Count: %lu\n", cpu_enter_cnt);
    fprintf(stderr, "CPU Exit Count: %lu\n", cpu_exit_cnt);
    fprintf(stderr, "SYSCALL Exit Count: %lu\n", syscall_exit_cnt);

    fprintf(stderr, "Time in QEMU: %jd.%09lds\n", tv_qemu.tv_sec, tv_qemu.tv_nsec);
    fprintf(stderr, "Time in Code: %jd.%09lds\n", tv_code.tv_sec, tv_code.tv_nsec);
    fprintf(stderr, "Translated ins nr: %lu, avg_trans_time: %.9f\n", total_trans_ins_nr, (tv_qemu.tv_sec + (double)tv_qemu.tv_nsec/1000000000)/total_trans_ins_nr);

    fprintf(stderr, "BBL: %ld, INS: %ld, Avg: %f INSs/BBL\n", bbl_exec_nr, ins_exec_nr, (double)ins_exec_nr / bbl_exec_nr);
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    Init();

    TRACE_AddInstrumentFunction(Trace, 0);

    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddCpuExecEnterFunction(cpu_exec_enter, 0);
    PIN_AddCpuExecExitFunction(cpu_exec_exit, 0);
 
    PIN_AddFiniFunction(Fini, 0);
 
    return 0;
}
