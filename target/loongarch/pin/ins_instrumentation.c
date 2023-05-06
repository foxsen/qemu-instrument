#include "ins_instrumentation.h"
#include "ins_inspection.h"
#include "pin_state.h"
#include <stdarg.h>
#include "../instrument/util/error.h"
#include "../instrument/tr_data.h"
#include "../instrument/regs.h"
#include "../instrument/env.h"
#include "../instrument/util/bitopts.h"
#include "../instrument/decoder/la_print.h"
#include "reg.h"

/* 插桩分析函数时，是否需要保存浮点上下文（保存了的话，coremark只有92...） */
bool save_fpr_regs = false;

BOOL PIN_Init(INT32 argc, CHAR** argv)
{
    return 0;
}

VOID PIN_InitSymbols(void)
{
    PIN_state.read_symbol = true;
}

void INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val)
{
    PIN_state.ins_cb = fun;
    PIN_state.ins_cb_val = val;
}

void TRACE_AddInstrumentFunction(TRACE_INSTRUMENT_CALLBACK fun, VOID *val)
{
    PIN_state.trace_cb = fun;
    PIN_state.trace_cb_val = val;
}

void IMG_AddInstrumentFunction(IMAGECALLBACK fun, VOID *val)
{
    PIN_state.img_cb = fun;
    PIN_state.img_cb_val = val;
}

void PIN_AddFiniFunction(FINI_CALLBACK fun, VOID *val)
{
    PIN_state.fini_cb = fun;
    PIN_state.fini_cb_val = val;
}

void PIN_AddSyscallEntryFunction(SYSCALL_ENTRY_CALLBACK fun, VOID *val)
{
    PIN_state.syscall_entry_cb = fun;
    PIN_state.syscall_entry_cb_val = val;
}

void PIN_AddSyscallExitFunction(SYSCALL_EXIT_CALLBACK fun, VOID *val)
{
    PIN_state.syscall_exit_cb = fun;
    PIN_state.syscall_exit_cb_val = val;
}

void PIN_AddCpuExecEnterFunction(CPU_EXEC_ENTER_CALLBACK fun, VOID *val)
{
    PIN_state.cpu_exec_enter_cb = fun;
    PIN_state.cpu_exec_enter_cb_val = val;
}

void PIN_AddCpuExecExitFunction(CPU_EXEC_EXIT_CALLBACK fun, VOID *val)
{
    PIN_state.cpu_exec_exit_cb = fun;
    PIN_state.cpu_exec_exit_cb_val = val;
}

/* Parse Instrument Arguments
 * 解析IARG，记录到callback结构体
 * 只支持整数类型的参数，最多8个
 * 只做最基本的“词法分析”，不做语法检查
 */
static ANALYSIS_CALL parse_iarg(IOBJECT object, IPOINT ipoint, AFUNPTR funptr, va_list va)
{
    ANALYSIS_CALL cb;
    IARG_TYPE type;
    int i = 0;
    /* 最多8个参数 */
    while (i < 8) {
        type = va_arg(va, IARG_TYPE);
        if (type == IARG_END) {
            break;
        }
        cb.arg[i].type = type;

        switch (type) {
        case IARG_ADDRINT:
            cb.arg[i].value = va_arg(va, ADDRINT);
            break;
        case IARG_PTR:
            cb.arg[i].value = (uint64_t)va_arg(va, VOID*);
            break;
        case IARG_BOOL:
            /* va_arg promote char to int */
            cb.arg[i].value = va_arg(va, INT);
            break;
        case IARG_UINT32:
            cb.arg[i].value = va_arg(va, UINT32);
            break;
        case IARG_UINT64:
            cb.arg[i].value = va_arg(va, UINT64);
            break;
        case IARG_INST_PTR:
            break;
        case IARG_REG_VALUE:
            /* 记录寄存器号，之后向cb传寄存器的值 */
            cb.arg[i].value = va_arg(va, ADDRINT);
            break;
        case IARG_REG_REFERENCE:
            /* 记录寄存器号，之后向cb传存放寄存器的值的指针，用户修改后[会]被写回真的寄存器env->reg */
            cb.arg[i].value = va_arg(va, UINT64);
            break;
        case IARG_REG_CONST_REFERENCE:
            /* 记录寄存器号，之后向cb传存放寄存器的值的指针，用户修改后[不会]被写回真的寄存器 */
            cb.arg[i].value = va_arg(va, UINT64);
            break;
        case IARG_MEMORYREAD_EA:
        case IARG_MEMORYWRITE_EA:
            /* 访存的虚拟地址 */
            break;
        case IARG_MEMORYREAD_SIZE:
        case IARG_MEMORYWRITE_SIZE:
            /* 访存大小 */
            break;
        case IARG_MEMORYREAD_PTR:
        case IARG_MEMORYWRITE_PTR:
            /* 等效于IARG_MEMORYxx_EA */
            /* 一种情况是用于addr被重写的情况，返回重写后的addr */
            break;
        case IARG_BRANCH_TAKEN:
            /* if branch is taken, return 1 */
        case IARG_BRANCH_TARGET_ADDR:
            /* assert INS_IsControlFlow() == true */
        case IARG_FALLTHROUGH_ADDR:
            /* assert INS_HasFallThrough() ==true */
        case IARG_EXECUTING:
            /* true if ins will be executed */
        case IARG_SYSCALL_NUMBER:
            break;
        case IARG_SYSARG_REFERENCE:
        case IARG_SYSARG_VALUE:
            /* 指示第几个arg(从0开始) */
            cb.arg[i].value = va_arg(va, UINT64);
            break;
        case IARG_SYSRET_VALUE:
        case IARG_SYSRET_ERRNO:
            break;
        case IARG_FUNCARG_CALLSITE_REFERENCE:
        case IARG_FUNCARG_CALLSITE_VALUE:
        case IARG_FUNCARG_ENTRYPOINT_REFERENCE:
        case IARG_FUNCARG_ENTRYPOINT_VALUE:
            /* 指示第几个arg(从0开始) */
            cb.arg[i].value = va_arg(va, UINT64);
            break;
        case IARG_FUNCRET_EXITPOINT_REFERENCE:
        case IARG_FUNCRET_EXITPOINT_VALUE:
        case IARG_RETURN_IP:
            break;
        case IARG_ORIG_FUNCPTR:
            break;
        case IARG_THREAD_ID:
            break;
        case IARG_MEMORYOP_EA:
        case IARG_MEMORYOP_SIZE:
        case IARG_MEMORYOP_PTR:
            cb.arg[i].value = va_arg(va, UINT64);
            break;
        case IARG_TSC:
            break;
        case IARG_MULTI_MEMORYACCESS_EA:
        case IARG_MULTI_ELEMENT_OPERAND:
        case IARG_REWRITE_SCATTERED_MEMOP:
        case IARG_EXPLICIT_MEMORY_EA:
        case IARG_PROTOTYPE:
        case IARG_CONTEXT:
        case IARG_CONST_CONTEXT:
        case IARG_PARTIAL_CONTEXT:
        case IARG_PRESERVE:
        case IARG_RETURN_REGS:
        case IARG_CALL_ORDER:
        case IARG_IARGLIST:
            lsassertm(0, "Not implemented IARG_TYPE. type=%d\n", type);
            break;
        case IARG_MEMORYREAD2_EA:
        case IARG_MEMORYREAD2_PTR:
        case IARG_FIRST_REP_ITERATION:
        case IARG_FAST_ANALYSIS_CALL:
        case IARG_MEMORYOP_MASKED_ON:
            lsassertm(0, "Unsupported IARG_TYPE in LoongArch. type=%d\n", type);
            break;
        case IARG_PREDICATE:
        case IARG_STACK_VALUE:
        case IARG_STACK_REFERENCE:
        case IARG_MEMORY_VALUE:
        case IARG_MEMORY_REFERENCE:
        case IARG_FILE_NAME:
        case IARG_LINE_NO:
        case IARG_LAST:
            lsassertm(0, "Reserved IARG_TYPE. type=%d\n", type);
            break;
        case IARG_INVALID:
        case IARG_END:
        default:
            lsassertm(0, "Invalid IARG_TYPE. type=%d\n", type);
            break;
        }

        ++i;
    }

    cb.func = funptr;
    cb.object = object;
    cb.ipoint = ipoint;
    cb.arg_cnt = i;
    return cb;
}

static const int arg_reg_map[] = {
    [0] = reg_a0,
    [1] = reg_a1,
    [2] = reg_a2,
    [3] = reg_a3,
    [4] = reg_a4,
    [5] = reg_a5,
    [6] = reg_a6,
    [7] = reg_a7,
};

/* 调用分析函数前，需要保存的寄存器。
 * 包括：直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp
 * TODO: reg_tp 要保存，要恢复成host的吗？我觉得要 */
static bool gpr_need_saved_in_instru(int gpr) {
    return gpr_is_mapped(gpr) && (reg_ra <= mapped_gpr(gpr) && mapped_gpr(gpr) <= reg_t8);
}

/* 调用分析函数时，还被直接映射的寄存器 */
static bool gpr_is_mapped_in_instru(int gpr) {
    return gpr_is_mapped(gpr) && !gpr_need_saved_in_instru(gpr);
}

static bool fpr_need_saved_in_instru(int fpr) {
    if (!save_fpr_regs) {
        return false;
    }
    return fpr_is_mapped(fpr) && (reg_f0 <= mapped_fpr(fpr) && mapped_fpr(fpr) <= reg_f23);
}

static bool fpr_is_mapped_in_instru(int fpr) {
    return fpr_is_mapped(fpr) && !fpr_need_saved_in_instru(fpr);
}

static void load_REG_to_gpr(INS INS, Ins *cur, REG REG, int reg)
{
    if (REG_is_gpr(REG)) {
        int gpr = REG_to_gpr(REG);
        if (gpr_is_mapped_in_instru(gpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, reg, mapped_gpr(gpr), 0));
        } else {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, reg, reg_env, env_offset_of_gpr(current_cpu, gpr)));
        }
    } else if (REG_is_fpr(REG)) {
        int fpr = REG_to_fpr(REG);
        if (fpr_is_mapped_in_instru(fpr)) {
            INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVFR2GR_D, reg, mapped_fpr(fpr)));
        } else {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, reg, reg_env, env_offset_of_fpr(current_cpu, fpr)));
        }
    } else {
        lsassertm(0, "invalid REG: not a GPR or FPR\n");
    }
}

/* store REG to memory(env) if REG is mapped */
static void store_REG_to_env(INS INS, Ins *cur, REG REG)
{
    if (REG_is_gpr(REG)) {
        int gpr = REG_to_gpr(REG);
        if (gpr_is_mapped_in_instru(gpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_ST_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(current_cpu, gpr)));
        }
    } else if (REG_is_fpr(REG)) {
        int fpr = REG_to_fpr(REG);
        if (fpr_is_mapped_in_instru(fpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_FST_D, mapped_fpr(fpr), reg_env, env_offset_of_fpr(current_cpu, fpr)));
        }
    } else {
        lsassertm(0, "invalid REG: not a GPR or FPR\n");
    }
}

/* get REG's address in memory(env) */
static void load_reg_env_ptr_to_reg(INS INS, Ins *cur, REG REG, int reg)
{
    if (REG_is_gpr(REG)) {
        /* ADDI_D use si12 is enough */
        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADDI_D, reg, reg_env, env_offset_of_gpr(current_cpu, REG_to_gpr(REG))));
    } else if (REG_is_fpr(REG)) {
        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADDI_D, reg, reg_env, env_offset_of_fpr(current_cpu, REG_to_fpr(REG))));
    } else {
        lsassertm(0, "invalid REG: not a GPR or FPR\n");
    }
}

static void set_iargs(const ANALYSIS_CALL *cb, INS INS, Ins *cur)
{
    /* IOBJECT object = cb->object; */
    IPOINT ipoint = cb->ipoint;
    /* AFUNPTR funptr = cb->func; */

    IR2_OPCODE op = INS->origin_ins->op;

    for (int i = cb->arg_cnt - 1; i >= 0; --i) {
        int arg_reg = arg_reg_map[i];
        IARG_TYPE arg_type = cb->arg[i].type;
        uint64_t arg_value = cb->arg[i].value;

        switch(arg_type) {
        case IARG_ADDRINT:
        case IARG_PTR:
        case IARG_BOOL:
        case IARG_UINT32:
        case IARG_UINT64:
            INS_load_imm64_before(INS, cur, arg_reg, arg_value);
            break;
        case IARG_INST_PTR:
            INS_load_imm64_before(INS, cur, arg_reg, INS_Address(INS));
            break;
        case IARG_REG_VALUE:
            load_REG_to_gpr(INS, cur, arg_value, arg_reg);
            break;
        case IARG_SYSCALL_NUMBER:
            lsassert(ipoint == IPOINT_BEFORE && op == LISA_SYSCALL);
            load_REG_to_gpr(INS, cur, REG_SYSCALL_NR, arg_reg);
            break;
        case IARG_SYSARG_VALUE:
            lsassert(ipoint == IPOINT_BEFORE && op == LISA_SYSCALL && arg_value < 7);
            load_REG_to_gpr(INS, cur, REG_SYSCALL_ARG0 + arg_value, arg_reg);
            break;
        case IARG_SYSRET_VALUE:
            lsassert(ipoint == IPOINT_AFTER && op == LISA_SYSCALL);
            load_REG_to_gpr(INS, cur, REG_SYSCALL_RET, arg_reg);
            break;
        case IARG_FUNCARG_CALLSITE_VALUE:
        case IARG_FUNCARG_ENTRYPOINT_VALUE:
            /* TODO assert at CALLSITE/ENTRYPOINT */
            lsassert(arg_value < 8);
            load_REG_to_gpr(INS, cur, REG_A0 + arg_value, arg_reg);
            break;
        case IARG_FUNCRET_EXITPOINT_VALUE:
            lsassert(INS_IsRet(INS));
            load_REG_to_gpr(INS, cur, REG_A0, arg_reg);
            break;
        case IARG_RETURN_IP:
            /* TODO assert at entrypoint */
            /* 所有的官方示例里都是对RTN使用的 */
            load_REG_to_gpr(INS, cur, REG_RA, arg_reg);
            break;
        case IARG_REG_REFERENCE:
            {
                REG REG = arg_value;
                /* store reg to memory if needed, it will be reload after calling analysis_call */
                store_REG_to_env(INS, cur, REG);
                load_reg_env_ptr_to_reg(INS, cur, REG, arg_reg);
            }
            break;
        case IARG_SYSARG_REFERENCE:
            {
                lsassert(ipoint == IPOINT_BEFORE && op == LISA_SYSCALL && arg_value < 7);
                REG REG = gpr_to_REG(reg_syscall_arg0 + arg_value);
                store_REG_to_env(INS, cur, REG);
                load_reg_env_ptr_to_reg(INS, cur, REG, arg_reg);
            }
            break;
        case IARG_REG_CONST_REFERENCE:
            lsassertm(0, "not implemented\n");
            break;
        case IARG_MEMORYREAD_EA:
        case IARG_MEMORYWRITE_EA:
        case IARG_MEMORYOP_EA:
            if (arg_type == IARG_MEMORYREAD_EA) {
                lsassertm(INS_IsMemoryRead(INS) && ipoint == IPOINT_BEFORE, "IARG_MEMORYREAD_EA only valid if INS_IsMemoryRead is true and at IPOINT_BEFORE\n");
            } else if (arg_type == IARG_MEMORYWRITE_EA) {
                lsassertm(INS_IsMemoryWrite(INS) && ipoint == IPOINT_BEFORE, "IARG_MEMORYWRITE_EA only valid if INS_IsMemoryWrite is true and at IPOINT_BEFORE\n");
            } else if (arg_type == IARG_MEMORYOP_EA) {
                lsassertm((INS_IsMemoryRead(INS) || INS_IsMemoryWrite(INS)) && ipoint == IPOINT_BEFORE, "IARG_MEMORYOP_EA only valid at IPOINT_BEFORE\n");
                lsassertm(arg_value == 0, "LoongArch ins only has one memOp\n");
            }

            /* TODO 原子操作AM*未实现 */
            switch (op) {
            case LISA_LD_B:
            case LISA_LD_H:
            case LISA_LD_W:
            case LISA_LD_D:
            case LISA_ST_B:
            case LISA_ST_H:
            case LISA_ST_W:
            case LISA_ST_D:
            case LISA_LD_BU:
            case LISA_LD_HU:
            case LISA_LD_WU:
            case LISA_FLD_S:
            case LISA_FST_S:
            case LISA_FLD_D:
            case LISA_FST_D:
            case LISA_PRELD:
            case LISA_VLD:
            case LISA_VST:
            case LISA_XVLD:
            case LISA_XVST:
            case LISA_LDL_W:
            case LISA_LDR_W:
            case LISA_LDL_D:
            case LISA_LDR_D:
            case LISA_STL_W:
            case LISA_STR_W:
            case LISA_STL_D:
            case LISA_STR_D:
                /* addr = rj + si12 */
                {
                    int rj = INS->origin_ins->opnd[1].val;
                    int si12 = INS->origin_ins->opnd[2].val;
                    if (gpr_is_mapped_in_instru(rj)) {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, mapped_gpr(rj), 0));
                    } else {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, arg_reg, reg_env, env_offset_of_gpr(current_cpu, rj)));
                    }
                    INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADDI_D, arg_reg, arg_reg, si12));
                }
                break;
            case LISA_LDX_B:
            case LISA_LDX_H:
            case LISA_LDX_W:
            case LISA_LDX_D:
            case LISA_LDX_BU:
            case LISA_LDX_HU:
            case LISA_LDX_WU:
            case LISA_STX_B:
            case LISA_STX_H:
            case LISA_STX_W:
            case LISA_STX_D:
            case LISA_FLDX_S:
            case LISA_FLDX_D:
            case LISA_FSTX_S:
            case LISA_FSTX_D:
            case LISA_VLDX:
            case LISA_VSTX:
            case LISA_XVLDX:
            case LISA_XVSTX:
                /* addr = rj + rk */
                {
                    int rj = INS->origin_ins->opnd[1].val;
                    int rk = INS->origin_ins->opnd[2].val;
                    if (gpr_is_mapped_in_instru(rj)) {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, mapped_gpr(rj), 0));
                    } else {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, arg_reg, reg_env, env_offset_of_gpr(current_cpu, rj)));
                    }
                    if (gpr_is_mapped_in_instru(rk)) {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, mapped_gpr(rk)));
                    } else {
                        int itemp = reg_alloc_itemp();
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, itemp, reg_env, env_offset_of_gpr(current_cpu, rk)));
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, itemp));
                        reg_free_itemp(itemp);
                    }
                }
                break;
            case LISA_LDGT_B:
            case LISA_LDGT_H:
            case LISA_LDGT_W:
            case LISA_LDGT_D:
            case LISA_LDLE_B:
            case LISA_LDLE_H:
            case LISA_LDLE_W:
            case LISA_LDLE_D:
            case LISA_STGT_B:
            case LISA_STGT_H:
            case LISA_STGT_W:
            case LISA_STGT_D:
            case LISA_STLE_B:
            case LISA_STLE_H:
            case LISA_STLE_W:
            case LISA_STLE_D:
            case LISA_FLDGT_S:
            case LISA_FLDGT_D:
            case LISA_FLDLE_S:
            case LISA_FLDLE_D:
            case LISA_FSTGT_S:
            case LISA_FSTGT_D:
            case LISA_FSTLE_S:
            case LISA_FSTLE_D:
                /* addr = rj */
                {
                    int rj = INS->origin_ins->opnd[1].val;
                    if (gpr_is_mapped_in_instru(rj)) {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, mapped_gpr(rj), 0));
                    } else {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, arg_reg, reg_env, env_offset_of_gpr(current_cpu, rj)));
                    }
                }
                break;
            case LISA_LDPTR_W:
            case LISA_LDPTR_D:
            case LISA_STPTR_W:
            case LISA_STPTR_D:
            case LISA_LL_W:
            case LISA_LL_D:
            case LISA_SC_W:
            case LISA_SC_D:
                /* addr = rj + (si14 << 2) */
                {
                    int rj = INS->origin_ins->opnd[1].val;
                    int si14 = INS->origin_ins->opnd[2].val;
                    if (gpr_is_mapped_in_instru(rj)) {
                        INS_load_imm64_before(INS, cur, arg_reg, sign_extend(si14 << 2, 16));
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, mapped_gpr(rj)));
                    } else {
                        int itemp = reg_alloc_itemp();
                        INS_load_imm64_before(INS, cur, arg_reg, sign_extend(si14 << 2, 16));
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, itemp, reg_env, env_offset_of_gpr(current_cpu, rj)));
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, itemp));
                        reg_free_itemp(itemp);
                    }
                }
                break;
            case LISA_PRELDX:
                /* addr = rj + sign_extend(rk[15:0]) */
                {
                    int rj = INS->origin_ins->opnd[1].val;
                    int rk = INS->origin_ins->opnd[2].val;
                    if (gpr_is_mapped_in_instru(rk)) {
                        INS_insert_ins_before(INS, cur, ins_create_2(LISA_EXT_W_H, arg_reg, mapped_gpr(rk)));
                    } else {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_H, arg_reg, reg_env, env_offset_of_gpr(current_cpu, rk)));
                    }
                    if (gpr_is_mapped_in_instru(rj)) {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, mapped_gpr(rj)));
                    } else {
                        int itemp = reg_alloc_itemp();
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, itemp, reg_env, env_offset_of_gpr(current_cpu, rj)));
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, itemp));
                        reg_free_itemp(itemp);
                    }
                }
                break;
            case LISA_VLDREPL_B:
            case LISA_XVLDREPL_B:
            case LISA_VLDREPL_H:
            case LISA_XVLDREPL_H:
            case LISA_VLDREPL_W:
            case LISA_XVLDREPL_W:
            case LISA_VLDREPL_D:
            case LISA_XVLDREPL_D:
            case LISA_VSTELM_B:
            case LISA_XVSTELM_B:
            case LISA_VSTELM_H:
            case LISA_XVSTELM_H:
            case LISA_VSTELM_W:
            case LISA_XVSTELM_W:
            case LISA_VSTELM_D:
            case LISA_XVSTELM_D:
                /* addr = rj + offset, offset <= 12bits */
                {
                    int rj = INS->origin_ins->opnd[1].val;
                    int offset = INS->origin_ins->opnd[2].val;
                    switch (op) {
                    case LISA_VLDREPL_B:
                    case LISA_XVLDREPL_B:
                    case LISA_VSTELM_B:
                    case LISA_XVSTELM_B:
                        break;
                    case LISA_VLDREPL_H:
                    case LISA_XVLDREPL_H:
                    case LISA_VSTELM_H:
                    case LISA_XVSTELM_H:
                        offset <<= 1;
                        break;
                    case LISA_VLDREPL_W:
                    case LISA_XVLDREPL_W:
                    case LISA_VSTELM_W:
                    case LISA_XVSTELM_W:
                        offset <<= 2;
                        break;
                    case LISA_VLDREPL_D:
                    case LISA_XVLDREPL_D:
                    case LISA_VSTELM_D:
                    case LISA_XVSTELM_D:
                        offset <<= 3;
                        break;
                    default:
                        lsassertm(0, "op is not VLDREPL/VSTELM series\n");
                    }

                    if (gpr_is_mapped_in_instru(rj)) {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, mapped_gpr(rj), 0));
                    } else {
                        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, arg_reg, reg_env, env_offset_of_gpr(current_cpu, rj)));
                    }
                    INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADDI_D, arg_reg, arg_reg, offset));
                }
                break;
            default:
                print_ins(INS->origin_ins);
                lsassertm(0, "unhandled op\n");
            }
            break;
        case IARG_MEMORYREAD_SIZE:
        case IARG_MEMORYWRITE_SIZE:
        case IARG_MEMORYOP_SIZE:
            lsassertm(arg_type != IARG_MEMORYOP_SIZE || arg_value == 0, "LoongArch ins only has one memOp\n");
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, reg_zero, INS_MemoryOperandSize(INS, 0)));
            break;
        case IARG_MEMORYREAD_PTR:
        case IARG_MEMORYWRITE_PTR:
        case IARG_MEMORYOP_PTR:
            /* 用于addr被重写的情况，返回重写后的addr */
            lsassertm(0, "Currently memory_operand rewrite is not supported, use IARG_MEMORYxxx_EA instead\n");
            break;
        case IARG_BRANCH_TAKEN:
        case IARG_EXECUTING:
            if (op_is_condition_branch(op)) {
                /*
                 * BEQ rj, rd, off
                 * ->
                 * ORI $arg_reg, $zero, 1
                 * BEQ rj, rd, 2        // jump over 2 ins if branch taken
                 * ORI $arg_reg, $zero, 0
                 */

                INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, reg_zero, 1));

                switch (op) {
                case LISA_BEQZ:
                case LISA_BNEZ:
                    {
                        int rj = INS->origin_ins->opnd[0].val;
                        if (gpr_is_mapped_in_instru(rj)) {
                            INS_insert_ins_before(INS, cur, ins_create_2(op, mapped_gpr(rj), 2));
                        } else {
                            int itemp = reg_alloc_itemp();
                            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, itemp, reg_env, env_offset_of_gpr(current_cpu, rj)));
                            INS_insert_ins_before(INS, cur, ins_create_2(op, itemp, 2));
                            reg_free_itemp(itemp);
                        }
                    }
                    break;
                case LISA_BCEQZ:
                case LISA_BCNEZ:
                    {
                        int fcc = INS->origin_ins->opnd[0].val;
                        if (save_fpr_regs) {
                            int itemp = reg_alloc_itemp();
                            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_B, itemp, reg_env, env_offset_of_fcc(current_cpu, fcc))); 
                            INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVGR2CF, fcc, itemp));
                            reg_free_itemp(itemp);
                        }
                        INS_insert_ins_before(INS, cur, ins_create_2(op, fcc, 2));
                    }
                    break;
                case LISA_BEQ:
                case LISA_BNE:
                case LISA_BLT:
                case LISA_BGE:
                case LISA_BLTU:
                case LISA_BGEU:
                    {
                        int rj = INS->origin_ins->opnd[0].val;
                        int rd = INS->origin_ins->opnd[1].val;
                        int opnd1;
                        int opnd2;

                        if (gpr_is_mapped_in_instru(rj)) {
                            opnd1 = mapped_gpr(rj);
                        } else {
                            opnd1 = reg_alloc_itemp();
                            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, opnd1, reg_env, env_offset_of_gpr(current_cpu, rj)));
                        }
                        if (gpr_is_mapped_in_instru(rd)) {
                            opnd2 = mapped_gpr(rd);
                        } else {
                            opnd2 = reg_alloc_itemp();
                            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, opnd2, reg_env, env_offset_of_gpr(current_cpu, rd)));
                        }
                        INS_insert_ins_before(INS, cur, ins_create_3(op, opnd1, opnd2, 2));

                        if (!gpr_is_mapped_in_instru(rj)) {
                            reg_free_itemp(opnd1);
                        }
                        if (!gpr_is_mapped_in_instru(rd)) {
                            reg_free_itemp(opnd2);
                        }
                    }
                    break;
                default:
                    lsassertm(0, "unhandled condition branch\n");
                    break;
                }
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, reg_zero, 0));
            } else if (op_is_branch(op)) {
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, reg_zero, 1));
            } else {
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_ORI, arg_reg, reg_zero, 0));
            }
            break;
        case IARG_BRANCH_TARGET_ADDR:
            lsassert(INS_IsControlFlow(INS));
            if (op_is_direct_branch(op)) {
                INS_load_imm64_before(INS, cur, arg_reg, ins_target_addr(INS->origin_ins, INS->pc));
            } else if (op_is_indirect_branch(op)) {
                int rj = INS->origin_ins->opnd[1].val;
                int si16 = INS->origin_ins->opnd[2].val;
                if (gpr_is_mapped_in_instru(rj)) {
                    INS_load_imm64_before(INS, cur, arg_reg, sign_extend(si16 << 2, 18));
                    INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, mapped_gpr(rj)));
                } else {
                    int itemp = reg_alloc_itemp();
                    INS_load_imm64_before(INS, cur, arg_reg, sign_extend(si16 << 2, 18));
                    INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, itemp, reg_env, env_offset_of_gpr(current_cpu, rj)));
                    INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, arg_reg, arg_reg, itemp));
                    reg_free_itemp(itemp);
                }
            } else {
                lsassertm(0, "not a branch ins\n");
            }
            break;
        case IARG_FALLTHROUGH_ADDR:
            lsassert(INS_HasFallThrough(INS));
            INS_load_imm64_before(INS, cur, arg_reg, INS->pc + 4);
            break;
        case IARG_THREAD_ID:
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, arg_reg, reg_env, env_offset_of_opaque(current_cpu)));
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, arg_reg, arg_reg, opaque_offset_of_ts_tid()));
            break;
        case IARG_TSC:
            /* NOTE: In LoongArch, We can only use RDTIME_D to read Stable Counter,
             * which has a low performance and low accuracy. What's the worse is 
             * every core has a Stable Counter, leads to a wrong counting. */
            INS_insert_ins_before(INS, cur, ins_create_2(LISA_RDTIME_D, arg_reg, reg_zero));
            break;
        case IARG_MULTI_MEMORYACCESS_EA:
        case IARG_MULTI_ELEMENT_OPERAND:
        case IARG_REWRITE_SCATTERED_MEMOP:
        case IARG_EXPLICIT_MEMORY_EA:
        case IARG_SYSRET_ERRNO:
        case IARG_FUNCARG_CALLSITE_REFERENCE:
        case IARG_FUNCARG_ENTRYPOINT_REFERENCE:
        case IARG_FUNCRET_EXITPOINT_REFERENCE:
        case IARG_ORIG_FUNCPTR:
        case IARG_PROTOTYPE:
        case IARG_CONTEXT:
        case IARG_CONST_CONTEXT:
        case IARG_PARTIAL_CONTEXT:
        case IARG_PRESERVE:
        case IARG_RETURN_REGS:
        case IARG_CALL_ORDER:
        case IARG_IARGLIST:
            lsassertm(0, "Not implemented IARG_TYPE. type=%d\n", arg_type);
            break;
        case IARG_MEMORYREAD2_EA:
        case IARG_MEMORYREAD2_PTR:
        case IARG_FIRST_REP_ITERATION:
        case IARG_FAST_ANALYSIS_CALL:
        case IARG_MEMORYOP_MASKED_ON:
            lsassertm(0, "Unsupported IARG_TYPE in LoongArch. type=%d\n", arg_type);
            break;
        case IARG_PREDICATE:
        case IARG_STACK_VALUE:
        case IARG_STACK_REFERENCE:
        case IARG_MEMORY_VALUE:
        case IARG_MEMORY_REFERENCE:
        case IARG_FILE_NAME:
        case IARG_LINE_NO:
        case IARG_LAST:
            lsassertm(0, "Reserved IARG_TYPE. type=%d\n", arg_type);
            break;
        case IARG_INVALID:
        case IARG_END:
        default:
            lsassertm(0, "Invalid IARG_TYPE. type=%d\n", arg_type);
            break;
        }
    }
}



static void iargs_delayed_action(const ANALYSIS_CALL *cb, INS INS, Ins *cur)
{
    /* IOBJECT object = cb->object; */
    /* IPOINT ipoint = cb->ipoint; */
    /* AFUNPTR funptr = cb->func; */
    /* IR2_OPCODE op = INS->origin_ins->op; */

    for (int i = cb->arg_cnt - 1; i >= 0; --i) {
        IARG_TYPE arg_type = cb->arg[i].type;
        uint64_t arg_value = cb->arg[i].value;

        switch(arg_type) {
        case IARG_REG_REFERENCE:
        case IARG_SYSARG_REFERENCE:
            {
                int gpr;
                switch (arg_type) {
                case IARG_REG_REFERENCE:
                    gpr = REG_to_gpr(arg_value);
                    break;
                case IARG_SYSARG_REFERENCE:
                    gpr = reg_syscall_arg0 + arg_value;
                    break;
                default:
                    lsassertm(0, "unhandled iarg\n");
                    break;
                }

                /* analysis_call modified the reg, so reload it to mapped gpr */
                if (gpr_is_mapped_in_instru(gpr)) {
                    INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(current_cpu, gpr)));
                }
            }
            break;
        default:
            break;
        }
    }
}



/* 调用分析函数前，保存直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
static void save_caller_saved_regs(INS INS, Ins *cur)
{
    /* save gpr */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr_need_saved_in_instru(gpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_ST_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(current_cpu, gpr)));
        }
    }
    /* save fpr */
    for (int fpr = 0; fpr < 32; ++fpr) {
        if (fpr_need_saved_in_instru(fpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_FST_D, mapped_fpr(fpr), reg_env, env_offset_of_fpr(current_cpu, fpr)));
        }
    }
    /* save fcc[8], fcsr0 */
    if (save_fpr_regs) {
        int reg_tmp = reg_t8;
        for (int fcc = 0; fcc < 8; ++fcc) {
            INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVCF2GR, reg_tmp, fcc));
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_ST_B, reg_tmp, reg_env, env_offset_of_fcc(current_cpu, fcc)));
        }
        INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVFCSR2GR, reg_tmp, reg_fcsr));
        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ST_W, reg_tmp, reg_env, env_offset_of_fscr0(current_cpu))); 
    }
}

/* 调用分析函数后，恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
static void load_caller_saved_regs(INS INS, Ins *cur)
{
    /* 恢复 fcc[8], fcsr0 */
    if (save_fpr_regs) {
        int reg_tmp = reg_t8;
        for (int fcc = 0; fcc < 8; ++fcc) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_B, reg_tmp, reg_env, env_offset_of_fcc(current_cpu, fcc)));
            INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVGR2CF, reg_tmp, fcc));
        }
        INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_W, reg_tmp, reg_env, env_offset_of_fscr0(current_cpu))); 
        INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVGR2FCSR, reg_tmp, reg_fcsr));
    }
    /* 恢复 fpr */
    for (int fpr = 0; fpr < 32; ++fpr) {
        if (fpr_need_saved_in_instru(fpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_FLD_D, mapped_fpr(fpr), reg_env, env_offset_of_fpr(current_cpu, fpr)));
        }
    }
    /* 恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr_need_saved_in_instru(gpr)) {
            INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(current_cpu, gpr)));
        }
    }
}

static Ins *init_callback_block(INS INS, Ins *cur)
{
    /* 1. 调用分析函数前，保存直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    save_caller_saved_regs(INS, cur);

    /* 2. 切换为host的sp,tp */
    INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(current_cpu)));
    INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, reg_tp, reg_env, env_offset_of_host_tp(current_cpu)));

    /* 插入分析函数调用的位置，此处可插入多个分析函数调用 */
    Ins *pos = cur->prev;

    /* 3. 调用分析函数后，恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    load_caller_saved_regs(INS, cur);

    return pos->next;
}

static Ins *get_next_cb_position(INS INS, IPOINT ipoint)
{
    /* 若对同一指令插入多个分析函数调用，我们只需在这些调用前后保存和恢复一次caller-saved寄存器 */
    if (ipoint == IPOINT_BEFORE) {
        if (INS->ibefore_next_cb == NULL) {
            INS->ibefore_next_cb = init_callback_block(INS, INS->first_ins);
        }
        return INS->ibefore_next_cb;
    }
    if (ipoint == IPOINT_AFTER) {
        if (INS->iafter_next_cb == NULL) {
            // 在INS末尾临时插一条nop，方便在nop之前插入指令
            INS_insert_ins_after(INS, INS->last_ins, ins_nop());
            INS->iafter_next_cb = init_callback_block(INS, INS->last_ins);
            INS_remove_ins(INS, INS->last_ins);
        }
        return INS->iafter_next_cb;
    }
    return NULL;
}

static void insert_callback(INS INS, Ins *cur, ANALYSIS_CALL *cb)
{
    /* 设置分析函数的参数 */
    set_iargs(cb, INS, cur);

    /* 插入对分析函数的调用 */
    int itemp_target = reg_alloc_itemp();
    INS_load_imm64_before(INS, cur, itemp_target, (uint64_t)cb->func);
    INS_insert_ins_before(INS, cur, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
    reg_free_itemp(itemp_target);

    /* 调用分析函数后的触发的事情，如IARG_REG_REFERENCE修改的寄存器需要写回 */
    iargs_delayed_action(cb, INS, cur);
}

VOID INS_InsertCall(INS INS, IPOINT ipoint, AFUNPTR funptr, ...)
{
    lsassertm(ipoint == IPOINT_BEFORE || INS_HasFallThrough(INS), "can not instrument at IPOINT_AFTER for unconditional branch\n");

    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_INS, ipoint, funptr, valist);
    va_end(valist);

    Ins *cur = get_next_cb_position(INS, ipoint);

    insert_callback(INS, cur, &cb);
}

VOID INS_InsertPredicatedCall (INS INS, IPOINT ipoint, AFUNPTR funptr,...)
{
    lsassertm(ipoint == IPOINT_BEFORE || INS_HasFallThrough(INS), "can not instrument at IPOINT_AFTER for unconditional branch\n");

    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_INS, ipoint, funptr, valist);
    va_end(valist);

    Ins *cur = get_next_cb_position(INS, ipoint);

    /* 根据指令的predicate决定是否跳过分析函数 */
    Ins *jmp = NULL;
    int offset_opnd_index = -1;
    IR2_OPCODE op = INS->origin_ins->op;
    switch (op) {
    case LISA_BEQZ:
    case LISA_BNEZ:
        {
            int rj = INS->origin_ins->opnd[0].val;
            if (gpr_is_mapped_in_instru(rj)) {
                jmp = ins_create_2(op, mapped_gpr(rj), 0);
                INS_insert_ins_before(INS, cur, jmp);
            } else {
                int itemp = reg_alloc_itemp();
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, itemp, reg_env, env_offset_of_gpr(current_cpu, rj)));
                jmp = ins_create_2(op, itemp, 0);
                INS_insert_ins_before(INS, cur, jmp);
                reg_free_itemp(itemp);
            }
            offset_opnd_index = 1;
        }
        break;
    case LISA_BCEQZ:
    case LISA_BCNEZ:
        {
            int fcc = INS->origin_ins->opnd[0].val;
            if (save_fpr_regs) {
                int itemp = reg_alloc_itemp();
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_B, itemp, reg_env, env_offset_of_fcc(current_cpu, fcc))); 
                INS_insert_ins_before(INS, cur, ins_create_2(LISA_MOVGR2CF, fcc, itemp));
                reg_free_itemp(itemp);
            }
            jmp = ins_create_2(op, fcc, 0);
            offset_opnd_index = 1;
            INS_insert_ins_before(INS, cur, jmp);
        }
        break;
    case LISA_BEQ:
    case LISA_BNE:
    case LISA_BLT:
    case LISA_BGE:
    case LISA_BLTU:
    case LISA_BGEU:
        {
            int rj = INS->origin_ins->opnd[0].val;
            int rd = INS->origin_ins->opnd[1].val;
            int opnd1;
            int opnd2;

            if (gpr_is_mapped_in_instru(rj)) {
                opnd1 = mapped_gpr(rj);
            } else {
                opnd1 = reg_alloc_itemp();
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, opnd1, reg_env, env_offset_of_gpr(current_cpu, rj)));
            }
            if (gpr_is_mapped_in_instru(rd)) {
                opnd2 = mapped_gpr(rd);
            } else {
                opnd2 = reg_alloc_itemp();
                INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, opnd2, reg_env, env_offset_of_gpr(current_cpu, rd)));
            }
            jmp = ins_create_3(op, opnd1, opnd2, 0);
            offset_opnd_index = 2;
            INS_insert_ins_before(INS, cur, jmp);

            if (!gpr_is_mapped_in_instru(rj)) {
                reg_free_itemp(opnd1);
            }
            if (!gpr_is_mapped_in_instru(rd)) {
                reg_free_itemp(opnd2);
            }
        }
        break;
    default:
        break;
    }

    insert_callback(INS, cur, &cb);
    
    /* 修正jmp跳过的指令数 */
    if (jmp != NULL ) {
        int offset = 0;
        Ins *i = jmp;
        while (i != cur) {
            ++offset;
            i = i->next;
        }
        jmp->opnd[offset_opnd_index].val = offset;
    }
}

VOID INS_InsertIfCall(INS INS, IPOINT ipoint, AFUNPTR funptr,...)
{
    lsassertm(ipoint == IPOINT_BEFORE || INS_HasFallThrough(INS), "can not instrument at IPOINT_AFTER for unconditional branch\n");

    /* 此处不插桩，推迟到INS_InsertThenCall()再插桩 */
    va_list valist;
    va_start(valist, funptr);
    PIN_instru_ctx.ins_if_call_cb = parse_iarg(IOBJECT_INS, ipoint, funptr, valist);
    PIN_instru_ctx.ins_if_call_valid = true;
    va_end(valist);
}

VOID INS_InsertThenCall (INS INS, IPOINT ipoint, AFUNPTR funptr,...)
{
    /* FIXME more strict validation */
    /* ThenCall必须紧接着IfCall调用 */
    /* 目前的设计是将IfCall的返回值放在$a0里，根据$a0决定是否调用thenCall */
    lsassertm(PIN_instru_ctx.ins_if_call_valid == true && PIN_instru_ctx.ins_if_call_cb.ipoint == ipoint, "should call INS_InsertIfCall before INS_InsertThenCall\n");
    ANALYSIS_CALL *if_cb = &PIN_instru_ctx.ins_if_call_cb;
    PIN_instru_ctx.ins_if_call_valid = false;

    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL then_cb = parse_iarg(IOBJECT_INS, ipoint, funptr, valist);
    va_end(valist);

    Ins *cur = get_next_cb_position(INS, ipoint);
    /* 1. If Call */
    insert_callback(INS, cur, if_cb);

    /* 2. 根据ifCall的返回值，决定是否执行thenCall */
    Ins *beqz = ins_create_2(LISA_BEQZ, reg_a0, 0);     // 后面会修正beqz的offset
    INS_insert_ins_before(INS, cur, beqz);

    /* 3. Then Call */
    insert_callback(INS, cur, &then_cb);
    
    /* 修正beqz跳过的指令数 */
    {
        int offset = 0;
        Ins *i = beqz;
        while (i != cur) {
            ++offset;
            i = i->next;
        }
        beqz->opnd[1].val = offset;
    }
}

VOID BBL_InsertCall(BBL bbl, IPOINT ipoint, AFUNPTR funptr, ...)
{
    lsassertm(ipoint == IPOINT_BEFORE || BBL_HasFallThrough(bbl), "can not instrument at IPOINT_AFTER for unconditional branch\n");

    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_BBL, ipoint, funptr, valist);
    va_end(valist);

    INS INS = bbl->ins_head;
    Ins *cur = get_next_cb_position(INS, ipoint);
    insert_callback(INS, cur, &cb);
}

VOID BBL_InsertIfCall (BBL bbl, IPOINT ipoint, AFUNPTR funptr,...)
{
    lsassertm(ipoint == IPOINT_BEFORE || BBL_HasFallThrough(bbl), "can not instrument at IPOINT_AFTER for unconditional branch\n");

    va_list valist;
    va_start(valist, funptr);
    PIN_instru_ctx.bbl_if_call_cb = parse_iarg(IOBJECT_BBL, ipoint, funptr, valist);
    PIN_instru_ctx.bbl_if_call_valid = true;
    va_end(valist);
}

VOID BBL_InsertThenCall (BBL BBL, IPOINT ipoint, AFUNPTR funptr,...)
{
    /* FIXME more strict validation */
    /* 目前的设计是ThenCall必须紧接着IfCall调用，因为IfCall的返回值放在$a0里 */
    lsassertm(PIN_instru_ctx.bbl_if_call_valid == true && PIN_instru_ctx.bbl_if_call_cb.ipoint == ipoint, "should call BBL_InsertIfCall before BBL_InsertThenCall\n");
    ANALYSIS_CALL *if_cb = &PIN_instru_ctx.bbl_if_call_cb;
    PIN_instru_ctx.bbl_if_call_valid = false;

    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL then_cb = parse_iarg(IOBJECT_BBL, ipoint, funptr, valist);
    va_end(valist);

    INS INS = BBL->ins_head;
    Ins *cur = get_next_cb_position(INS, ipoint);
    /* 1. If Call */
    insert_callback(INS, cur, if_cb);

    /* 2. 根据ifCall的返回值，决定是否执行thenCall */
    Ins *beqz = ins_create_2(LISA_BEQZ, reg_a0, 0);     // 后面会修正beqz的offset
    INS_insert_ins_before(INS, cur, beqz);

    /* 3. Then Call */
    insert_callback(INS, cur, &then_cb);
    
    /* 修正beqz跳过的指令数 */
    {
        int offset = 0;
        Ins *i = beqz;
        while (i != cur) {
            ++offset;
            i = i->next;
        }
        beqz->opnd[1].val = offset;
    }
}

VOID TRACE_InsertCall(TRACE trace, IPOINT action, AFUNPTR funptr, ...)
{
    assert(0);
}

static void _RTN_InsertCall(INS INS, ANALYSIS_CALL *cb)
{
    Ins *cur = get_next_cb_position(INS, IPOINT_BEFORE);
    insert_callback(INS, cur, cb);
}


#include "symbol.h"
/* 只是记录插桩信息，直到翻译时，遇到函数入口时再插桩 */
VOID RTN_InsertCall(RTN rtn, IPOINT ipoint, AFUNPTR funptr, ...)
{
    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_RTN, ipoint, funptr, valist);
    va_end(valist);

    /* recored cb */
    if (ipoint == IPOINT_BEFORE) {
        RTN_add_entry_cb(rtn, &cb);
    } else if (ipoint == IPOINT_AFTER) {
        RTN_add_exit_cb(rtn, &cb);
    } else {
        lsassertm(0, "invalid rtn ipoint\n");
    }
}

/* 检查是否需要函数插桩 */
/* 非PinAPI */
/* TODO 移到pin_state.h */
VOID RTN_instrument(TRACE trace)
{
    BBL bbl = TRACE_BblHead(trace);
    INS ins = BBL_InsHead(bbl);

    /* 检查trace的第一条指令是否为要函数插桩的函数入口 */
    int cb_cnt;
    ANALYSIS_CALL * cbs = RTN_get_entry_cbs(ins->pc, &cb_cnt);
    for (int i = 0; i < cb_cnt; ++i) {
        _RTN_InsertCall(ins, &cbs[i]);
    }

    /* 检查trace的最后一条指令是否为要函数插桩的函数出口 */
    bbl = TRACE_BblTail(trace);
    ins = BBL_InsTail(bbl);

    Ins * origin_ins = ins->origin_ins;
    if (origin_ins->op == LISA_JIRL &&
        origin_ins->opnd[0].val == reg_zero &&
        origin_ins->opnd[1].val == reg_ra &&
        origin_ins->opnd[2].val == 0) {

        cbs = RTN_get_exit_cbs(ins->pc, &cb_cnt);
        for (int i = 0; i < cb_cnt; ++i) {
            _RTN_InsertCall(ins, &cbs[i]);
        }
    }
}



/* Add imm to the value pointed by ptr, non-atmoic add */
static VOID insert_inline_add(INS INS, Ins *cur, VOID* ptr, UINT64 imm)
{
    int reg_addr = reg_alloc_itemp();
    int reg_val = reg_alloc_itemp();
    INS_load_imm64_before(INS, cur, reg_addr, (uint64_t)ptr);
    INS_insert_ins_before(INS, cur, ins_create_3(LISA_LD_D, reg_val, reg_addr, 0));
    if (imm <= 0x7ff || imm >= 0xfffff800) {
        /* if imm only 12 bits, use ADDI.D */
        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADDI_D, reg_val, reg_val, imm));
    } else {
        int reg_imm = reg_alloc_itemp();
        INS_load_imm64_before(INS, cur, reg_imm, imm);
        INS_insert_ins_before(INS, cur, ins_create_3(LISA_ADD_D, reg_val, reg_val, reg_imm));
        reg_free_itemp(reg_imm);
    }
    INS_insert_ins_before(INS, cur, ins_create_3(LISA_ST_D, reg_val, reg_addr, 0));
    reg_free_itemp(reg_addr);
    reg_free_itemp(reg_val);
}

static VOID insert_inline_add_atomic(INS INS, Ins *cur, VOID* ptr, UINT64 imm)
{
    int reg_addr = reg_alloc_itemp();
    int reg_imm = reg_alloc_itemp();
    INS_load_imm64_before(INS, cur, reg_addr, (uint64_t)ptr);
    INS_load_imm64_before(INS, cur, reg_imm, imm);
    INS_insert_ins_before(INS, cur, ins_create_3(LISA_AMADD_D, reg_zero, reg_imm, reg_addr));
    reg_free_itemp(reg_addr);
    reg_free_itemp(reg_imm);
}

VOID INS_InsertInlineAdd(INS INS, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic)
{
    Ins *cur;
    if (action == IPOINT_BEFORE) {
        cur = INS->first_ins;
    } else if (action == IPOINT_AFTER) {
        // 在INS末尾临时插一条nop，方便在nop之前插入指令
        INS_insert_ins_after(INS, INS->last_ins, ins_nop());
        cur = INS->last_ins;
    } else {
        lsassertm(0, "Unsupported IPOINT type\n");
        return;
    }

    if (!atomic) {
        insert_inline_add(INS, cur, ptr, imm);
    } else {
        insert_inline_add_atomic(INS, cur, ptr, imm);
    }

    if (action == IPOINT_AFTER) {
        INS_remove_ins(INS, INS->last_ins);
    }
}

VOID BBL_InsertInlineAdd(BBL bbl, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic)
{
    lsassertm(action == IPOINT_BEFORE, "can not insert an inline-add after a BBL\n");

    if (!atomic) {
        insert_inline_add(bbl->ins_head, bbl->ins_head->first_ins, ptr, imm);
    } else {
        insert_inline_add_atomic(bbl->ins_head, bbl->ins_head->first_ins, ptr, imm);
    }
}


ADDRINT PIN_GetContextReg(const CONTEXT* ctxt, REG reg)
{
    /* FIXME mapped gpr is not in env */
    assert(REG_is_gpr(reg));
    return ctxt->env->gpr[REG_to_gpr(reg)];
}

VOID PIN_SetContextReg (CONTEXT *ctxt, REG reg, ADDRINT val)
{
    /* FIXME mapped gpr is not in env */
    assert(REG_is_gpr(reg));
    ctxt->env->gpr[REG_to_gpr(reg)] = val;
}

ADDRINT PIN_GetSyscallNumber (const CONTEXT *ctxt, SYSCALL_STANDARD std)
{
    return ctxt->env->gpr[reg_syscall_nr];
}

VOID PIN_SetSyscallNumber (CONTEXT *ctxt, SYSCALL_STANDARD std, ADDRINT val)
{
    ctxt->env->gpr[reg_syscall_nr] = val;
}

ADDRINT PIN_GetSyscallArgument (const CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum)
{
    /* SYSCALL_STANDARD is not used */
    assert(argNum <= 6);
    return ctxt->env->gpr[reg_syscall_arg0 + argNum];
}

VOID PIN_SetSyscallArgument (CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum, ADDRINT val)
{
    assert(argNum <= 6);
    ctxt->env->gpr[reg_syscall_arg0 + argNum] = val;
}

ADDRINT PIN_GetSyscallReturn(const CONTEXT* ctxt, SYSCALL_STANDARD std)
{
    return ctxt->env->gpr[reg_syscall_ret];
}

VOID PIN_SetSyscallReturn (CONTEXT *ctxt, SYSCALL_STANDARD std, ADDRINT val)
{
    ctxt->env->gpr[reg_syscall_ret] = val;
}
