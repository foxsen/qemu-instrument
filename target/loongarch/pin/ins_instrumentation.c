#include "ins_instrumentation.h"
#include "ins_inspection.h"
#include "pin_state.h"
#include <stdarg.h>
#include "../instrument/error.h"
#include "../instrument/ins.h"
#include "../instrument/regs.h"
#include "../instrument/env.h"
#include "../instrument/bitopts.h"
#include "../instrument/decoder/la_print.h"

/* 插桩分析函数时，是否需要保存浮点上下文（保存了的话，coremark只有92...） */
bool save_fpr_regs = false;


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

#include <stdarg.h>
/* Parse Instrument Arguments
 * 只支持整数类型的参数，最多8个
 */
static ANALYSIS_CALL parse_iarg(IOBJECT object, IPOINT action, AFUNPTR funptr, va_list va)
{
    lsassert(action == IPOINT_BEFORE);

    ANALYSIS_CALL cb;
    IARG_TYPE type;
    int i = 0;
    /* 最多8个参数 */
    while (i < 8) {
        type = va_arg(va, IARG_TYPE);
        cb.arg[i].type = type;

        if (type == IARG_END) {
            break;
        }

        switch (type) {
        case IARG_ADDRINT:
            cb.arg[i].val = va_arg(va, ADDRINT);
            break;
        case IARG_PTR:
            cb.arg[i].val = (uint64_t)va_arg(va, VOID*);
            break;
        case IARG_BOOL:
            /* va_arg promote char to int */
            cb.arg[i].val = va_arg(va, INT);
            break;
        case IARG_UINT32:
            cb.arg[i].val = va_arg(va, UINT32);
            break;
        case IARG_UINT64:
            cb.arg[i].val = va_arg(va, UINT64);
            break;
        case IARG_INST_PTR:
            break;
        case IARG_REG_VALUE:
            /* 记录寄存器号，之后向cb传寄存器的值 */
            cb.arg[i].val = va_arg(va, ADDRINT);
            break;
        case IARG_REG_REFERENCE:
            /* 记录寄存器号，之后向cb传存放寄存器的值的指针，用户修改后[会]被写回真的寄存器env->reg */
            cb.arg[i].val = va_arg(va, UINT64);
            break;
        case IARG_REG_CONST_REFERENCE:
            /* 记录寄存器号，之后向cb传存放寄存器的值的指针，用户修改后[不会]被写回真的寄存器 */
            cb.arg[i].val = va_arg(va, UINT64);
            break;
        case IARG_MEMORYREAD_EA:
        case IARG_MEMORYWRITE_EA:
        case IARG_MEMORYOP_EA:
            /* 访存的虚拟地址 */
        case IARG_MEMORYREAD_SIZE:
        case IARG_MEMORYWRITE_SIZE:
        case IARG_MEMORYOP_SIZE:
            /* 访存大小 */
        case IARG_MEMORYREAD_PTR:
        case IARG_MEMORYWRITE_PTR:
        case IARG_MEMORYOP_PTR:
            /* 一种情况是用于addr被重写的情况，返回重写后的addr */
        case IARG_BRANCH_TAKEN:
            /* if branch is taken, return 1 */
        case IARG_BRANCH_TARGET_ADDR:
            /* assert INS_IsControlFlow() == true */
        case IARG_FALLTHROUGH_ADDR:
            /* assert INS_HasFallThrough() ==true */
        case IARG_EXECUTING:
            /* 1 if ins will be executed */
        case IARG_SYSCALL_NUMBER:
            break;
        case IARG_SYSARG_REFERENCE:
        case IARG_SYSARG_VALUE:
            /* 指示第几个arg(从0开始) */
            cb.arg[i].val = va_arg(va, UINT64);
            break;
        case IARG_SYSRET_VALUE:
        case IARG_SYSRET_ERRNO:
            break;
        case IARG_FUNCARG_CALLSITE_REFERENCE:
        case IARG_FUNCARG_CALLSITE_VALUE:
        case IARG_FUNCARG_ENTRYPOINT_REFERENCE:
        case IARG_FUNCARG_ENTRYPOINT_VALUE:
            /* 指示第几个arg(从0开始) */
            cb.arg[i].val = va_arg(va, UINT64);
            break;
        case IARG_FUNCRET_EXITPOINT_REFERENCE:
        case IARG_FUNCRET_EXITPOINT_VALUE:
        case IARG_RETURN_IP:
            break;
        case IARG_THREAD_ID:
            lsassert(0);
            break;
        case IARG_TSC:
            break;
        case IARG_INVALID:
        default:
            lsassertm(0, "Invalid IARG_TYPE %d\n", type);
            break;
        }

        ++i;
    }

    cb.func = funptr;
    cb.object = object;
    cb.action = action;
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
 * TODO: reg_tp 要保存，要恢复成host的吗 */
static bool gpr_need_saved_in_instru(int gpr) {
    return gpr_is_mapped(gpr) && (reg_ra <= mapped_gpr(gpr) && mapped_gpr(gpr) <= reg_t8);
}

/* 调用分析时，还被直接映射的寄存器 */
static bool gpr_is_mapped_in_instru(int gpr) {
    return gpr_is_mapped(gpr) && !gpr_need_saved_in_instru(gpr);
}

/* 调用分析函数前，保存直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
static void save_caller_saved_regs(Ins *cur, int *ins_nr)
{
    /* gpr */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr_need_saved_in_instru(gpr)) {
            ins_insert_before(cur, ins_create_3(LISA_ST_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(current_cpu, gpr)));
            ++(*ins_nr);
        }
    }
    /* save fpr[0,23], fcc[8], fcsr0 */
    if (save_fpr_regs) {
        int reg_tmp = reg_t8;
        for (int fpr = 0; fpr < 24; ++fpr) {
            ins_insert_before(cur, ins_create_3(LISA_FST_D, fpr, reg_env, env_offset_of_fpr(current_cpu, fpr)));
            ++(*ins_nr);
        }
        for (int fcc = 0; fcc < 8; ++fcc) {
            ins_insert_before(cur, ins_create_2(LISA_MOVCF2GR, reg_tmp, fcc));
            ins_insert_before(cur, ins_create_3(LISA_ST_B, reg_tmp, reg_env, env_offset_of_fcc(current_cpu, fcc)));
            *ins_nr += 2;
        }
        ins_insert_before(cur, ins_create_2(LISA_MOVFCSR2GR, reg_tmp, reg_fcsr));
        ins_insert_before(cur, ins_create_3(LISA_ST_W, reg_tmp, reg_env, env_offset_of_fscr0(current_cpu))); 
        *ins_nr += 2;
    }
}

/* 调用分析函数后，恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
static void restore_caller_saved_regs(Ins *cur, int *ins_nr)
{
    /* 恢复 fpr[0,23], fcc[8], fcsr0 */
    if (save_fpr_regs) {
        int reg_tmp = reg_t8;
        for (int fpr = 0; fpr < 24; ++fpr) {
            ins_insert_before(cur, ins_create_3(LISA_FLD_D, fpr, reg_env, env_offset_of_fpr(current_cpu, fpr)));
            ++(*ins_nr);
        }
        for (int fcc = 0; fcc < 8; ++fcc) {
            ins_insert_before(cur, ins_create_3(LISA_LD_B, reg_tmp, reg_env, env_offset_of_fcc(current_cpu, fcc)));
            ins_insert_before(cur, ins_create_2(LISA_MOVGR2CF, reg_tmp, fcc));
            *ins_nr += 2;
        }
        ins_insert_before(cur, ins_create_3(LISA_LD_W, reg_tmp, reg_env, env_offset_of_fscr0(current_cpu))); 
        ins_insert_before(cur, ins_create_2(LISA_MOVGR2FCSR, reg_tmp, reg_fcsr));
        *ins_nr += 2;
    }

    /* 恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    for (int gpr = 0; gpr < 32; ++gpr) {
        if (gpr_need_saved_in_instru(gpr)) {
            ins_insert_before(cur, ins_create_3(LISA_LD_D, mapped_gpr(gpr), reg_env, env_offset_of_gpr(current_cpu, gpr)));
            ++(*ins_nr);
        }
    }
}

VOID INS_InsertCall(INS ins, IPOINT action, AFUNPTR funptr, ...)
{
    /* TODO: support IPONT_AFTER */
    lsassert(action == IPOINT_BEFORE);

    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_INS, action, funptr, valist);
    va_end(valist);


    Ins *cur = ins->first_ins;
    IR2_OPCODE op = ins->origin_ins->op;
    int ins_nr = 0;


    /* 调用分析函数前，保存直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    save_caller_saved_regs(cur, &ins_nr);


    /* 1. 设置分析函数的参数 */
    int itemp1 = reg_invalid;
    int itemp2 = reg_invalid;
    int gpr = reg_invalid;
    int fcc = reg_invalid;
    for (int i = cb.arg_cnt - 1; i >= 0; --i) {
        int argi = arg_reg_map[i];
        IARG_TYPE type = cb.arg[i].type;
        switch(type) {
        case IARG_ADDRINT:
        case IARG_PTR:
        case IARG_BOOL:
        case IARG_UINT32:
        case IARG_UINT64:
            ins_nr += ins_insert_before_li_d(cur, argi, cb.arg[i].val);
            break;
        case IARG_INST_PTR:
            ins_nr += ins_insert_before_li_d(cur, argi, INS_Address(ins));
            break;
        case IARG_REG_VALUE:
            /* only support gpr for now */
            gpr = cb.arg[i].val;
            assert(gpr < reg_end);
            if (gpr_is_mapped_in_instru(gpr)) {
                ins_insert_before(cur, ins_create_3(LISA_OR, argi, mapped_gpr(gpr), reg_zero));
            } else {
                ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, gpr)));
            }
            ++ins_nr;
            break;
        case IARG_REG_REFERENCE:
        case IARG_REG_CONST_REFERENCE:
            lsassert(0);
            break;
        case IARG_MEMORYREAD_EA:
        case IARG_MEMORYWRITE_EA:
        case IARG_MEMORYOP_EA:
            lsassertm(action == IPOINT_BEFORE, "IARG_MEMORYOP_EA only valid at IPOINT_BEFORE\n");
            /* TODO: 还有一些指令没实现 */
            /* TODO: 非边界访存指令都未实现 */
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
            case LISA_PRELD:
            case LISA_FLD_S:
            case LISA_FST_S:
            case LISA_FLD_D:
            case LISA_FST_D:
            case LISA_VLD:
            case LISA_VST:
            case LISA_XVLD:
            case LISA_XVST:
                gpr = cb.arg[i].val;
                if (gpr_is_mapped_in_instru(gpr)) {
                    ins_insert_before(cur, ins_create_3(LISA_OR, argi, mapped_gpr(gpr), reg_zero));
                } else {
                    ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                }
                /* NOTE: make sure the offset is si12 */
                ins_insert_before(cur, ins_create_3(LISA_ADDI_D, argi, argi, ins->origin_ins->opnd[2].val));
                ins_nr += 2;
                break;
            default:
                print_ins(ins->origin_ins);
                lsassertm(0, "unhandled op\n");
            }
            break;
        case IARG_MEMORYREAD_SIZE:
        case IARG_MEMORYWRITE_SIZE:
        case IARG_MEMORYOP_SIZE:
            ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, INS_MemoryOperandSize(ins, 0)));
            ++ins_nr;
            break;
        case IARG_MEMORYREAD_PTR:
        case IARG_MEMORYWRITE_PTR:
        case IARG_MEMORYOP_PTR:
            /* 用于addr被重写的情况，返回重写后的addr */
            lsassert(0);
            break;
        case IARG_BRANCH_TAKEN:
            if (op == LISA_SYSCALL || op == LISA_BREAK) {
                lsassertm(0, "is syscall/break a branch?\n");
            } else if (op_is_condition_branch(op)) {
                /*
                 * BEQ rj, rd, off
                 * ->
                 * ORI $argi, $zero, 1
                 * BEQ rj, rd, 2        // jump over 2 ins if branch taken
                 * ORI $argi, $zero, 0
                 */
                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 1));
                ++ins_nr;

                switch (op) {
                case LISA_BEQZ:
                case LISA_BNEZ:
                    itemp1 = reg_alloc_itemp();
                    gpr = ins->origin_ins->opnd[0].val;
                    if (gpr_is_mapped_in_instru(gpr)) {
                        ins_insert_before(cur, ins_create_3(LISA_OR, itemp1, mapped_gpr(gpr), reg_zero));
                        ++ins_nr;
                    } else {
                        ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                        ++ins_nr;
                    }
                    ins_insert_before(cur, ins_create_2(op, itemp1, 2));
                    ++ins_nr;
                    reg_free_itemp(itemp1);
                    break;
                case LISA_BCEQZ:
                case LISA_BCNEZ:
                    itemp1 = reg_alloc_itemp();
                    fcc = ins->origin_ins->opnd[0].val;
                    if (save_fpr_regs) {
                        ins_append_3(LISA_LD_B, itemp1, reg_env, env_offset_of_fcc(current_cpu, fcc)); 
                        ins_append_2(LISA_MOVGR2CF, fcc, itemp1);
                        ins_nr += 2;
                    }
                    ins_insert_before(cur, ins_create_2(op, itemp1, 2));
                    ++ins_nr;
                    reg_free_itemp(itemp1);
                    break;
                case LISA_BEQ:
                case LISA_BNE:
                case LISA_BLT:
                case LISA_BGE:
                case LISA_BLTU:
                case LISA_BGEU:
                    /* FIXME */
                    itemp1 = reg_alloc_itemp();
                    gpr = ins->origin_ins->opnd[0].val;
                    if (gpr_is_mapped_in_instru(gpr)) {
                        ins_insert_before(cur, ins_create_3(LISA_OR, itemp1, mapped_gpr(gpr), reg_zero));
                        ++ins_nr;
                    } else {
                        ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                        ++ins_nr;
                    }

                    itemp2 = reg_alloc_itemp();
                    gpr = ins->origin_ins->opnd[1].val;
                    if (gpr_is_mapped_in_instru(gpr)) {
                        ins_insert_before(cur, ins_create_3(LISA_OR, itemp1, mapped_gpr(gpr), reg_zero));
                        ++ins_nr;
                    } else {
                        ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                        ++ins_nr;
                    }
                    ins_insert_before(cur, ins_create_3(op, itemp1, itemp2, 2));
                    ++ins_nr;

                    reg_free_itemp(itemp1);
                    reg_free_itemp(itemp2);
                    break;
                default:
                    lsassertm(0, "invalid ins: not a condition branch\n");
                    break;
                }

                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 0));
                ++ins_nr;
            } else if (op_is_branch(op)) {
                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 1));
                ++ins_nr;
            } else {
                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 0));
                ++ins_nr;
            }
            break;
        case IARG_BRANCH_TARGET_ADDR:
            if (op_is_direct_branch(op)) {
                ins_nr += ins_insert_before_li_d(cur, argi, ins_target_addr(ins->origin_ins));
            } else if (op_is_indirect_branch(op)) {
                itemp1 = reg_alloc_itemp();
                ins_nr += ins_insert_before_li_d(cur, itemp1, sign_extend(ins->origin_ins->opnd[2].val << 2, 18));
                /* FIXME */
                lsassertm(0, "FIXME: check gpr is mapped");
                ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, ins->origin_ins->opnd[1].val)));
                ins_insert_before(cur, ins_create_3(LISA_ADD_D, argi, argi, itemp1));
                ins_nr += 2;
                reg_free_itemp(itemp1);
            } else {
                lsassertm(0, "not a branch ins\n");
            }
            break;
        case IARG_FALLTHROUGH_ADDR:
            lsassert(INS_HasFallThrough(ins));
            ins_nr += ins_insert_before_li_d(cur, argi, ins->pc + 4);
            break;
        case IARG_EXECUTING:
            /* TODO: LA中似乎没有 CMOVcc, FCMOVcc, REP */
            /* return 1 if ins will be executed */
            lsassert(0);
            break;
        case IARG_SYSCALL_NUMBER:
            /* FIXME */
            lsassertm(0, "FIXME: check gpr is mapped");
            lsassert(action == IPOINT_BEFORE && op == LISA_SYSCALL);
            ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, reg_a7)));
            ++ins_nr;
            break;
        case IARG_SYSARG_REFERENCE:
            lsassert(0);
            break;
        case IARG_SYSARG_VALUE:
            /* FIXME */
            lsassertm(0, "FIXME: check gpr is mapped");
            lsassert(action == IPOINT_BEFORE && op == LISA_SYSCALL);
            lsassert(cb.arg[i].val < 7);
            ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, reg_a0 + cb.arg[i].val)));
            break;

        default:
            lsassertm(0, "Invalid IARG_TYPE %d\n", cb.arg[i].type);
            break;
        }
    }


    /* 切换为host的sp,tp */
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(current_cpu)));
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_tp, reg_env, env_offset_of_host_tp(current_cpu)));
    ins_nr += 2;

    /* 3. 插入对分析函数的调用 */
    int itemp_target = reg_alloc_itemp();
    ins_nr += ins_insert_before_li_d(cur, itemp_target, (uint64_t)funptr);
    ins_insert_before(cur, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
    ++ins_nr;
    reg_free_itemp(itemp_target);

    /* 调用分析函数后，恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    restore_caller_saved_regs(cur, &ins_nr);

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}

VOID BBL_InsertCall(BBL bbl, IPOINT action, AFUNPTR funptr, ...)
{
    /* TODO: merge this function to INS_InsertCall */
    /* TODO: support IPONT_AFTER */
    lsassert(action == IPOINT_BEFORE);

    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_BBL, action, funptr, valist);
    va_end(valist);

    INS ins = bbl->ins_head;
    Ins *cur = ins->first_ins;
    IR2_OPCODE op = ins->origin_ins->op;
    int ins_nr = 0;


    /* 调用分析函数前，保存直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    save_caller_saved_regs(cur, &ins_nr);

    /* 1. 设置分析函数的参数 */
    int itemp1 = reg_invalid;
    int itemp2 = reg_invalid;
    int gpr = reg_invalid;
    int fcc = reg_invalid;
    for (int i = cb.arg_cnt - 1; i >= 0; --i) {
        int argi = arg_reg_map[i];
        IARG_TYPE type = cb.arg[i].type;
        switch(type) {
        case IARG_ADDRINT:
        case IARG_PTR:
        case IARG_BOOL:
        case IARG_UINT32:
        case IARG_UINT64:
            ins_nr += ins_insert_before_li_d(cur, argi, cb.arg[i].val);
            break;
        case IARG_INST_PTR:
            ins_nr += ins_insert_before_li_d(cur, argi, INS_Address(ins));
            break;
        case IARG_REG_VALUE:
            /* only support gpr for now */
            assert(cb.arg[i].val < reg_end);
            ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, cb.arg[i].val)));
            ++ins_nr;
            break;
        case IARG_REG_REFERENCE:
        case IARG_REG_CONST_REFERENCE:
            lsassert(0);
            break;
        case IARG_MEMORYREAD_EA:
        case IARG_MEMORYWRITE_EA:
        case IARG_MEMORYOP_EA:
            lsassertm(action == IPOINT_BEFORE, "IARG_MEMORYOP_EA only valid at IPOINT_BEFORE\n");
            /* TODO: 还有一些指令没实现 */
            /* TODO: 非边界访存指令都未实现 */
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
            case LISA_PRELD:
            case LISA_FLD_S:
            case LISA_FST_S:
            case LISA_FLD_D:
            case LISA_FST_D:
            case LISA_VLD:
            case LISA_VST:
            case LISA_XVLD:
            case LISA_XVST:
                ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, cb.arg[i].val)));
                ins_insert_before(cur, ins_create_3(LISA_ADDI_D, argi, argi, ins->origin_ins->opnd[2].val));
                ins_nr += 2;
                break;
            default:
                lsassertm(0, "unhandled op\n");
            }
            break;
        case IARG_MEMORYREAD_SIZE:
        case IARG_MEMORYWRITE_SIZE:
        case IARG_MEMORYOP_SIZE:
            ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, INS_MemoryOperandSize(ins, 0)));
            ++ins_nr;
            break;
        case IARG_MEMORYREAD_PTR:
        case IARG_MEMORYWRITE_PTR:
        case IARG_MEMORYOP_PTR:
            /* 用于addr被重写的情况，返回重写后的addr */
            lsassert(0);
            break;
        case IARG_BRANCH_TAKEN:
            if (op == LISA_SYSCALL || op == LISA_BREAK) {
                lsassertm(0, "is syscall/break a branch?\n");
            } else if (op_is_condition_branch(op)) {
                /*
                 * BEQ rj, rd, off
                 * ->
                 * ORI $argi, $zero, 1
                 * BEQ rj, rd, 2        // jump over 2 ins if branch taken
                 * ORI $argi, $zero, 0
                 */
                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 1));
                ++ins_nr;

                switch (op) {
                case LISA_BEQZ:
                case LISA_BNEZ:
                    itemp1 = reg_alloc_itemp();
                    gpr = ins->origin_ins->opnd[0].val;
                    if (gpr_is_mapped_in_instru(gpr)) {
                        ins_insert_before(cur, ins_create_3(LISA_OR, itemp1, mapped_gpr(gpr), reg_zero));
                        ++ins_nr;
                    } else {
                        ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                        ++ins_nr;
                    }
                    ins_insert_before(cur, ins_create_2(op, itemp1, 2));
                    ++ins_nr;
                    reg_free_itemp(itemp1);
                    break;
                case LISA_BCEQZ:
                case LISA_BCNEZ:
                    itemp1 = reg_alloc_itemp();
                    fcc = ins->origin_ins->opnd[0].val;
                    if (save_fpr_regs) {
                        ins_append_3(LISA_LD_B, itemp1, reg_env, env_offset_of_fcc(current_cpu, fcc)); 
                        ins_append_2(LISA_MOVGR2CF, fcc, itemp1);
                        ins_nr += 2;
                    }
                    ins_insert_before(cur, ins_create_2(op, itemp1, 2));
                    ++ins_nr;
                    reg_free_itemp(itemp1);
                    break;
                case LISA_BEQ:
                case LISA_BNE:
                case LISA_BLT:
                case LISA_BGE:
                case LISA_BLTU:
                case LISA_BGEU:
                    /* FIXME */
                    itemp1 = reg_alloc_itemp();
                    gpr = ins->origin_ins->opnd[0].val;
                    if (gpr_is_mapped_in_instru(gpr)) {
                        ins_insert_before(cur, ins_create_3(LISA_OR, itemp1, mapped_gpr(gpr), reg_zero));
                        ++ins_nr;
                    } else {
                        ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                        ++ins_nr;
                    }

                    itemp2 = reg_alloc_itemp();
                    gpr = ins->origin_ins->opnd[1].val;
                    if (gpr_is_mapped_in_instru(gpr)) {
                        ins_insert_before(cur, ins_create_3(LISA_OR, itemp1, mapped_gpr(gpr), reg_zero));
                        ++ins_nr;
                    } else {
                        ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, gpr)));
                        ++ins_nr;
                    }
                    ins_insert_before(cur, ins_create_3(op, itemp1, itemp2, 2));
                    ++ins_nr;

                    reg_free_itemp(itemp1);
                    reg_free_itemp(itemp2);
                    break;
                default:
                    lsassertm(0, "invalid ins: not a condition branch\n");
                    break;
                }

                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 0));
                ++ins_nr;
            } else if (op_is_branch(op)) {
                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 1));
                ++ins_nr;
            } else {
                ins_insert_before(cur, ins_create_3(LISA_ORI, argi, reg_zero, 0));
                ++ins_nr;
            }
            break;
        case IARG_BRANCH_TARGET_ADDR:
            if (op_is_direct_branch(op)) {
                ins_nr += ins_insert_before_li_d(cur, argi, ins_target_addr(ins->origin_ins));
            } else if (op_is_indirect_branch(op)) {
                itemp1 = reg_alloc_itemp();
                ins_nr += ins_insert_before_li_d(cur, itemp1, sign_extend(ins->origin_ins->opnd[2].val << 2, 18));
                ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, ins->origin_ins->opnd[1].val)));
                ins_insert_before(cur, ins_create_3(LISA_ADD_D, argi, argi, itemp1));
                ins_nr += 2;
                reg_free_itemp(itemp1);
            } else {
                lsassertm(0, "not a branch ins\n");
            }
            break;
        case IARG_FALLTHROUGH_ADDR:
            lsassert(INS_HasFallThrough(ins));
            ins_nr += ins_insert_before_li_d(cur, argi, ins->pc + 4);
            break;
        case IARG_EXECUTING:
            /* TODO: LA中似乎没有 CMOVcc, FCMOVcc, REP */
            /* return 1 if ins will be executed */
            lsassert(0);
            break;
        case IARG_SYSCALL_NUMBER:
            lsassert(action == IPOINT_BEFORE && op == LISA_SYSCALL);
            ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, reg_a7)));
            ++ins_nr;
            break;
        case IARG_SYSARG_REFERENCE:
            lsassert(0);
            break;
        case IARG_SYSARG_VALUE:
            lsassert(action == IPOINT_BEFORE && op == LISA_SYSCALL);
            lsassert(cb.arg[i].val < 7);
            ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, reg_a0 + cb.arg[i].val)));
            break;

        default:
            lsassertm(0, "Invalid IARG_TYPE %d\n", cb.arg[i].type);
            break;
        }

    }

    /* 切换为host的sp,tp */
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(current_cpu)));
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_tp, reg_env, env_offset_of_host_tp(current_cpu)));
    ins_nr += 2;

    /* 3. 插入对分析函数的调用 */
    int itemp_target = reg_alloc_itemp();
    ins_nr += ins_insert_before_li_d(cur, itemp_target, (uint64_t)funptr);
    ins_insert_before(cur, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
    ++ins_nr;
    reg_free_itemp(itemp_target);

    /* 调用分析函数后，恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    restore_caller_saved_regs(cur, &ins_nr);

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}


VOID TRACE_InsertCall(TRACE trace, IPOINT action, AFUNPTR funptr, ...)
{
    assert(0);
}

static void RTN_instrument_enrty(INS ins, ANALYSIS_CALL cb)
{
    Ins *cur = ins->first_ins;
    /* IR2_OPCODE op = ins->origin_ins->op; */
    int ins_nr = 0;

    /* 调用分析函数前，保存直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    save_caller_saved_regs(cur, &ins_nr);

    /* 1. 设置分析函数的参数 */
    int gpr = reg_invalid;
    for (int i = cb.arg_cnt - 1; i >= 0; --i) {
        int argi = arg_reg_map[i];
        IARG_TYPE type = cb.arg[i].type;
        switch(type) {
        case IARG_ADDRINT:
        case IARG_PTR:
        case IARG_BOOL:
        case IARG_UINT32:
        case IARG_UINT64:
            ins_nr += ins_insert_before_li_d(cur, argi, cb.arg[i].val);
            break;
        case IARG_INST_PTR:
            ins_nr += ins_insert_before_li_d(cur, argi, INS_Address(ins));
            break;
        case IARG_FUNCARG_ENTRYPOINT_VALUE:
            assert(cb.action == IPOINT_BEFORE);
            gpr = arg_reg_map[cb.arg[i].val];
            assert(reg_a0 <= gpr && gpr <= reg_a7);
            if (gpr_is_mapped_in_instru(gpr)) {
                ins_insert_before(cur, ins_create_3(LISA_OR, argi, mapped_gpr(gpr), reg_zero));
            } else {
                ins_insert_before(cur, ins_create_3(LISA_LD_D, argi, reg_env, env_offset_of_gpr(current_cpu, gpr)));
            }
            ++ins_nr;
            break;
        default:
            lsassertm(0, "Invalid IARG_TYPE %d\n", cb.arg[i].type);
            break;
        }
    }

    /* 2. 切换为host的sp,tp */
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_sp, reg_env, env_offset_of_host_sp(current_cpu)));
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_tp, reg_env, env_offset_of_host_tp(current_cpu)));
    ins_nr += 2;

    /* 3. 插入对分析函数的调用 */
    int itemp_target = reg_alloc_itemp();
    ins_nr += ins_insert_before_li_d(cur, itemp_target, (uint64_t)cb.func);
    ins_insert_before(cur, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
    ++ins_nr;
    reg_free_itemp(itemp_target);

    /* 调用分析函数后，恢复直接映射到了“调用者保存寄存器”的寄存器，以及sp,tp */
    restore_caller_saved_regs(cur, &ins_nr);

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}


    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}

#include "symbol.h"
/* 只是记录插桩信息，直到翻译时，遇到函数入口时再插桩 */
VOID RTN_InsertCall(RTN rtn, IPOINT action, AFUNPTR funptr, ...)
{
    /* Parse Instrument Arguments to cb */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(IOBJECT_RTN, action, funptr, valist);
    va_end(valist);

    /* recored the cb */
    RTN_save_cb(rtn.addr, cb);
}

/* 检查是否需要函数插桩
 * 非PinAPI */
VOID RTN_instrument(TRACE trace)
{
    BBL bbl = TRACE_BblHead(trace);
    if (!BBL_Valid(bbl)) {
        return;
    }
    INS ins = BBL_InsHead(bbl);
    if (!INS_Valid(ins)) {
        return;
    }

    /* 检查trace的第一条指令是否为要函数插桩的函数入口 */
    uintptr_t pc = ins->pc;
    int cb_nr;
    ANALYSIS_CALL * cbs = RTN_get_cbs(pc, &cb_nr);
    for (int i = 0; i < cb_nr; ++i) {
        RTN_insert_callback(ins, cbs[i]);
    }
}



/* add imm to value in ptr, not atmoic */
static VOID insert_inline_add(INS ins, VOID* ptr, UINT64 imm)
{
    Ins *cur = ins->first_ins;
    int ins_nr = 0;

    int reg_addr = reg_alloc_itemp();
    int reg_val = reg_alloc_itemp();
    ins_nr += ins_insert_before_li_d(cur, reg_addr, (uint64_t)ptr);
    ins_insert_before(cur, ins_create_3(LISA_LD_D, reg_val, reg_addr, 0));
    ++ins_nr;
    if (imm <= 0x7ff || imm >= 0xfffff800) {
        /* if imm only 12 bits, use ADDI.D */
        ins_insert_before(cur, ins_create_3(LISA_ADDI_D, reg_val, reg_val, imm));
        ++ins_nr;
    } else {
        int reg_imm = reg_alloc_itemp();
        ins_nr += ins_insert_before_li_d(cur, reg_imm, (uint64_t)imm);
        ins_insert_before(cur, ins_create_3(LISA_ADD_D, reg_val, reg_val, reg_imm));
        ++ins_nr;
        reg_free_itemp(reg_imm);
    }
    ins_insert_before(cur, ins_create_3(LISA_ST_D, reg_val, reg_addr, 0));
    ++ins_nr;
    reg_free_itemp(reg_addr);
    reg_free_itemp(reg_val);

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}

static VOID insert_inline_add_atomic(INS ins, VOID* ptr, UINT64 imm)
{
    Ins *cur = ins->first_ins;
    int ins_nr = 0;

    int reg_addr = reg_alloc_itemp();
    int reg_imm = reg_alloc_itemp();
    ins_nr += ins_insert_before_li_d(cur, reg_addr, (uint64_t)ptr);
    ins_nr += ins_insert_before_li_d(cur, reg_imm, (uint64_t)imm);
    ins_insert_before(cur, ins_create_3(LISA_AMADD_D, reg_zero, reg_imm, reg_addr));
    ++ins_nr;
    reg_free_itemp(reg_addr);
    reg_free_itemp(reg_imm);

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}

VOID INS_InsertInlineAdd(INS ins, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic)
{
    /* TODO: support IPONT_AFTER */
    lsassert(action == IPOINT_BEFORE);

    if (!atomic) {
        insert_inline_add(ins, ptr, imm);
    } else {
        insert_inline_add_atomic(ins, ptr, imm);
    }
}

VOID BBL_InsertInlineAdd(BBL bbl, IPOINT action, VOID* ptr, UINT64 imm, BOOL atomic)
{
    /* TODO: support IPONT_AFTER */
    lsassert(action == IPOINT_BEFORE);

    if (!atomic) {
        insert_inline_add(bbl->ins_head, ptr, imm);
    } else {
        insert_inline_add_atomic(bbl->ins_head, ptr, imm);
    }
}

ADDRINT PIN_GetSyscallNumber (const CONTEXT *ctxt, SYSCALL_STANDARD std)
{
    return ctxt->env->gpr[11];
}

VOID PIN_SetSyscallNumber (CONTEXT *ctxt, SYSCALL_STANDARD std, ADDRINT val)
{
    ctxt->env->gpr[11] = val;
}

ADDRINT PIN_GetSyscallArgument (const CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum)
{
    assert(0);
    return -1;
}

VOID PIN_SetSyscallArgument (CONTEXT *ctxt, SYSCALL_STANDARD std, UINT32 argNum, ADDRINT val)
{
    assert(0);
}
