#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../../instrument/decoder/ir2.h"
#include "../ins_inspection.h"

using namespace std;

#define NUM_INSTR_DESTINATIONS 1
#define NUM_INSTR_SOURCES 3


typedef struct trace_instr_format {
    unsigned long long int ip;  // instruction pointer (program counter) value
    unsigned long long int destination_memory[NUM_INSTR_DESTINATIONS]; // output memory
    unsigned long long int source_memory[NUM_INSTR_SOURCES];           // input memory
    unsigned long long ret_val;
    unsigned int inst;
    //unsigned short op;
    unsigned char is_branch;    // is this branch
    unsigned char branch_taken; // if so, is this taken

    unsigned char destination_registers[NUM_INSTR_DESTINATIONS]; // output registers
    unsigned char source_registers[NUM_INSTR_SOURCES];           // input registers
} trace_instr_format_t;


static FILE* trace_file;
#define TRACE_BUFFER_CAP (1 << 22)
static trace_instr_format_t trace_buffer[TRACE_BUFFER_CAP];
static uint64_t trace_buffer_index;
static void flush_trace_buffer(void) {
    fwrite(trace_buffer, trace_buffer_index, sizeof(trace_instr_format_t), trace_file);
    trace_buffer_index = 0;
}

const char* getenv_str(const char* __name, const char* __default) {
    const char* env = getenv(__name);
    return env ? env : __default;
}

static UINT64 icount = 0;
static UINT64 icount_begin = 1000000;
static UINT64 icount_end = 2000000;

void put_trace(trace_instr_format_t* trace) {
    if (trace_buffer_index == TRACE_BUFFER_CAP) {
        flush_trace_buffer();
    }
    trace_buffer[trace_buffer_index] = *trace;
    ++ trace_buffer_index;
    /*
    fprintf(stderr, "pc:%llx dm:%llx, sm:%llx w:%d s0:%d s1:%d s2:%d ret_val:%llx br:%d taken:%d\n", \
        trace->ip, trace->destination_memory[0], trace->source_memory[0], trace->destination_registers[0], \
        trace->source_registers[0], trace->source_registers[1], trace->source_registers[2], trace->ret_val, trace->is_branch, trace->branch_taken);
    */
}

#define reg_rw_w(rw) (unsigned char)(rw & 0xff)
#define reg_rw_r0(rw) (unsigned char)((rw >> 8) & 0xff)
#define reg_rw_r1(rw) (unsigned char)((rw >> 16) & 0xff)
#define reg_rw_r2(rw) (unsigned char)((rw >> 24) & 0xff)

static void dump_guest_memory_reg(void) {
    if (icount == icount_begin) {
        fprintf(stderr, "champsim TRACE_DUMP_MEMORY_REG\n");
        PIN_DumpGuestMemory(getenv_str("TRACE_DUMP_MEMORY", "memory.bin"));
        PIN_DumpGuestReg(getenv_str("TRACE_DUMP_REG", "regfile.txt"));
    }
}

static UINT64 ins_load_addr;
static VOID ins_load_before(UINT64 pc, UINT64 addr)
{
    ins_load_addr = addr;
}

static VOID ins_load_after(UINT64 pc, UINT64 reg_rw, UINT64 ret_val, UINT32 inst/*, UINT16 op*/)
{
    dump_guest_memory_reg();
    if (icount < icount_begin) {
        return;
    }
    if (icount >= icount_end) {
        flush_trace_buffer();
        exit(0);
    }
    trace_instr_format_t trace = {
        .ip = pc,
        .source_memory = {ins_load_addr},
        .ret_val = ret_val,
        .inst = inst,
        //.op = op,
        .destination_registers = {reg_rw_w(reg_rw)},
        .source_registers = {reg_rw_r0(reg_rw), reg_rw_r1(reg_rw), reg_rw_r2(reg_rw)},
    };
    put_trace(&trace);
}

static VOID ins_store(UINT64 pc, UINT64 addr, UINT64 reg_rw, UINT32 inst/*, UINT16 op*/)
{
    dump_guest_memory_reg();
    if (icount < icount_begin) {
        return;
    }
    if (icount >= icount_end) {
        flush_trace_buffer();
        exit(0);
    }
    trace_instr_format_t trace = {
        .ip = pc,
        .destination_memory = {addr},
        .inst = inst,
        //.op = op,
        .destination_registers = {reg_rw_w(reg_rw)},
        .source_registers = {reg_rw_r0(reg_rw), reg_rw_r1(reg_rw), reg_rw_r2(reg_rw)},
    };
    put_trace(&trace);
}

static VOID ins_br(UINT64 pc, UINT64 taken, UINT64 reg_rw, UINT64 ret_val, UINT32 inst/*, UINT16 op*/)
{
    dump_guest_memory_reg();
    if (icount < icount_begin) {
        return;
    }
    if (icount >= icount_end) {
        flush_trace_buffer();
        exit(0);
    }
    trace_instr_format_t trace = {
        .ip = pc,
        .ret_val = pc + 4,
        .inst = inst,
        //.op = op,
        .is_branch = 1,
        .branch_taken = (unsigned char)taken,
        .destination_registers = {reg_rw_w(reg_rw)},
        .source_registers = {reg_rw_r0(reg_rw), reg_rw_r1(reg_rw), reg_rw_r2(reg_rw)},
    };
    put_trace(&trace);
}

static VOID ins_others(UINT64 pc, UINT64 reg_rw, UINT64 ret_val, UINT32 inst/*, UINT16 op*/)
{
    dump_guest_memory_reg();
    if (icount < icount_begin) {
        return;
    }
    if (icount >= icount_end) {
        flush_trace_buffer();
        exit(0);
    }
    trace_instr_format_t trace = {
        .ip = pc,
        .ret_val = ret_val,
        .inst = inst,
        //.op = op,
        .destination_registers = {reg_rw_w(reg_rw)},
        .source_registers = {reg_rw_r0(reg_rw), reg_rw_r1(reg_rw), reg_rw_r2(reg_rw)},
    };
    put_trace(&trace);
}

static uint64_t get_reg_rw(INS ins) {
    long destination_registers = 0;
    for (int i = 0; i < ins->origin_ins->opnd_count; ++i) {
        LISA_REG_ACCESS_TYPE access_type = get_reg_access_type(ins->origin_ins, i);
        if (access_type == GPR_WRITE || access_type == GPR_READWRITE) {
            destination_registers = ins->origin_ins->opnd[i].val;
            break;
        } else if (access_type == FPR_WRITE || access_type == FPR_READWRITE) {
            destination_registers = ins->origin_ins->opnd[i].val + 32;
            break;
        }
    }
    int read_index = 0;
    long source_registers[4] = {0};
    for (int i = 0; i < ins->origin_ins->opnd_count; ++i) {
        LISA_REG_ACCESS_TYPE access_type = get_reg_access_type(ins->origin_ins, i);
        if (access_type == GPR_READ || access_type == GPR_READWRITE) {
            source_registers[read_index] = ins->origin_ins->opnd[i].val;
            read_index ++;
        } else if (access_type == FPR_READ || access_type == FPR_READWRITE) {
            source_registers[read_index] = ins->origin_ins->opnd[i].val + 32;
            read_index ++;
        }
    }
    return destination_registers | source_registers[0] << 8 | source_registers[1] << 16 | source_registers[2] << 24;
}


static VOID Instruction(INS ins, VOID* v)
{
    long rw = get_reg_rw(ins);
    long reg_w = rw & 0xff;
    /* Note: REG currenly only supports GRP and FPR */
    REG REG_W;
    if (reg_w < 32) {
        REG_W = gpr_to_REG(reg_w);
    } else {
        REG_W = fpr_to_REG(reg_w - 32);
    }
    unsigned int inst = ins->opcode;
    //unsigned short op = ins->origin_ins->op;
    if (INS_IsMemoryRead(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ins_load_before, IARG_UINT64, ins->pc, IARG_MEMORYOP_EA, 0, IARG_END);
        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)ins_load_after, IARG_UINT64, ins->pc, IARG_UINT64, rw, IARG_REG_VALUE, REG_W, IARG_UINT64, inst,/* op,*/ IARG_END);
    } else if (INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ins_store, IARG_UINT64, ins->pc, IARG_MEMORYOP_EA, 0, IARG_UINT64, rw, IARG_UINT64, inst,/* op,*/ IARG_END);
    } else if (INS_IsBranch(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ins_br, IARG_UINT64, ins->pc, IARG_BRANCH_TAKEN, IARG_UINT64, rw, IARG_REG_VALUE, REG_W, IARG_UINT64, inst,/* op,*/ IARG_END);
    } else {
        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)ins_others, IARG_UINT64, ins->pc, IARG_UINT64, rw, IARG_REG_VALUE, REG_W, IARG_UINT64, inst,/* op,*/ IARG_END);
    }
    INS_InsertInlineAdd(ins, IPOINT_BEFORE, &icount, 1, false);
}

static VOID Init(VOID)
{
    const char* trace_file_env = getenv("TRACE_FILE");
    const char* trace_file_name = trace_file_env ? trace_file_env : "champsim.trace";
    trace_file = fopen(trace_file_name, "wb");
    if (!trace_file) {
        perror(trace_file_name);
        exit(EXIT_FAILURE);
    }
    const char* icount_begin_env = getenv("ICOUNT_BEGIN");
    if (icount_begin_env) {
        icount_begin = atoll(icount_begin_env);
    }
    const char* icount_end_env = getenv("ICOUNT_END");
    if (icount_end_env) {
        icount_end = atoll(icount_end_env);
    }
    fprintf(stderr, "%s %ld %ld\n", trace_file_name, icount_begin, icount_end);
}

static VOID Fini(INT32 code, VOID* v)
{
    flush_trace_buffer();
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
