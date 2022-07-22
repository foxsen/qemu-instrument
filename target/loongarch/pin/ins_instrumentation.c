#include "ins_instrumentation.h"
#include "ins_inspection.h"
#include "pin_state.h"
#include <stdarg.h>
#include "../instrument/error.h"
#include "../instrument/ins.h"
#include "../instrument/regs.h"
#include "../instrument/env.h"
#include "../instrument/bitopts.h"


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

typedef struct analysis_call_arg {
    IARG_TYPE type;
    uint64_t val;
} iarg_t[8];

typedef struct analysis_call {
    AFUNPTR fun;
    int cnt;    // arg count
    iarg_t arg;
} ANALYSIS_CALL;


#include <stdarg.h>
/* Parse Instrument Arguments
 * 只支持整数类型的参数，最多8个
 */
ANALYSIS_CALL parse_iarg(IPOINT action, AFUNPTR funptr, va_list va);
ANALYSIS_CALL parse_iarg(IPOINT action, AFUNPTR funptr, va_list va)
{
    lsassert(action == IPOINT_BEFORE);

    ANALYSIS_CALL cb;
    IARG_TYPE type;
    int i = 0;
    /* 最多8个参数 */
    while (i < 8) {
        type = va_arg(va, IARG_TYPE);
        cb.arg[i].type = type;

        if (type == IARG_END)
            break;

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
            /* ??? */
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

    cb.fun = funptr;
    cb.cnt = i;
    return cb;
}

const int arg_reg_map[] = {
    [0] = reg_a0,
    [1] = reg_a1,
    [2] = reg_a2,
    [3] = reg_a3,
    [4] = reg_a4,
    [5] = reg_a5,
    [6] = reg_a6,
    [7] = reg_a7,
};

VOID INS_InsertCall(INS ins, IPOINT action, AFUNPTR funptr, ...)
{
    lsassert(action == IPOINT_BEFORE);

    /* Parse Instrument Arguments */
    va_list valist;
    va_start(valist, funptr);
    ANALYSIS_CALL cb = parse_iarg(action, funptr, valist);
    va_end(valist);

    /* 1. 设置分析函数的参数 */
    Ins *cur = ins->first_ins;
    IR2_OPCODE op = ins->origin_ins->op;
    int ins_nr = 0;
    int itemp1 = reg_invalid;
    int itemp2 = reg_invalid;
    for (int i = cb.cnt - 1; i >= 0; --i) {
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
            /* 非边界访存指令都未实现 */
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
                case LISA_BCEQZ:
                case LISA_BCNEZ:
                    itemp1 = reg_alloc_itemp();
                    ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, ins->origin_ins->opnd[0].val)));
                    ins_insert_before(cur, ins_create_2(op, itemp1, 2));
                    ins_nr += 2;
                    reg_free_itemp(itemp1);
                    break;
                case LISA_BEQ:
                case LISA_BNE:
                case LISA_BLT:
                case LISA_BGE:
                case LISA_BLTU:
                case LISA_BGEU:
                    itemp1 = reg_alloc_itemp();
                    itemp2 = reg_alloc_itemp();
                    ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp1, reg_env, env_offset_of_gpr(current_cpu, ins->origin_ins->opnd[0].val)));
                    ins_insert_before(cur, ins_create_3(LISA_LD_D, itemp2, reg_env, env_offset_of_gpr(current_cpu, ins->origin_ins->opnd[1].val)));
                    ins_insert_before(cur, ins_create_3(op, itemp1, itemp2, 2));
                    ins_nr += 3;
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

    /* 2. 对于直接映射的寄存器，保存映射到了t0~t8的寄存器（调用者保存的寄存器） */
    for (int i = 0; i < 32; ++i) {
        int gpr = reg_gpr_map[i];
        if (reg_t0 <= gpr && gpr <= reg_t8) {
            ins_insert_before(cur, ins_create_3(LISA_ST_D, gpr, reg_env, env_offset_of_gpr(current_cpu, i)));
            ++ins_nr;
        }
    }

    /* 3. 调用插桩函数 */
    int itemp_target = reg_alloc_itemp();
    ins_nr += ins_insert_before_li_d(cur, itemp_target, (uint64_t)funptr);
    ins_insert_before(cur, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
    ++ins_nr;
    reg_free_itemp(itemp_target);

    /* 4. 对于直接映射的寄存器，恢复映射到了t0~t8的寄存器（调用者保存的寄存器） */
    for (int i = 0; i < 32; ++i) {
        int gpr = reg_gpr_map[i];
        if (reg_t0 <= gpr && gpr <= reg_t8) {
            ins_insert_before(cur, ins_create_3(LISA_LD_D, gpr, reg_env, env_offset_of_gpr(current_cpu, i)));
            ++ins_nr;
        }
    }

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->len += ins_nr;
}

VOID BBL_InsertCall(BBL bbl, IPOINT action, AFUNPTR funptr, ...)
{
    lsassert(action == IPOINT_BEFORE);

    IARG_TYPE type;
    uint64_t args[8];
    int num = 0;

    va_list valist;
    va_start(valist, funptr);
    bool finish = false;
    while (!finish && num < 8) {
        type = va_arg(valist, IARG_TYPE);
        /* TODO: add more type */
        /* va_arg promote argument whose wide is less than int, so char in valist will became int */
        switch (type) {
        case IARG_END:
            finish = true;
            break;
        case IARG_ADDRINT:
            args[num++] = va_arg(valist, ADDRINT);
            break;
        case IARG_PTR:
            args[num++] = (uint64_t)va_arg(valist, VOID*);
            break;
        case IARG_BOOL:
            args[num++] = va_arg(valist, INT);
            break;
        case IARG_UINT32:
            args[num++] = va_arg(valist, UINT32);
            break;
        case IARG_UINT64:
            args[num++] = va_arg(valist, UINT64);
            break;
        case IARG_INVALID:
        default:
            lsassertm(0, "Invalid IARG_TYPE %d\n", type);
            break;
        }
    }
    va_end(valist);

    Ins *cur = bbl->ins_head->first_ins;
    int ins_nr = 0;
    int itemp_target = reg_alloc_itemp();
    switch (num) {
    case 8:
        /* TODO: save the args into mem could use less ins and also save space, but ld from men maybe slow */
        ins_nr += ins_insert_before_li_d(cur, reg_a7, args[7]);
        __attribute__((fallthrough));
    case 7:
        ins_nr += ins_insert_before_li_d(cur, reg_a6, args[6]);
        __attribute__((fallthrough));
    case 6:
        ins_nr += ins_insert_before_li_d(cur, reg_a5, args[5]);
        __attribute__((fallthrough));
    case 5:
        ins_nr += ins_insert_before_li_d(cur, reg_a4, args[4]);
        __attribute__((fallthrough));
    case 4:
        ins_nr += ins_insert_before_li_d(cur, reg_a3, args[3]);
        __attribute__((fallthrough));
    case 3:
        ins_nr += ins_insert_before_li_d(cur, reg_a2, args[2]);
        __attribute__((fallthrough));
    case 2:
        ins_nr += ins_insert_before_li_d(cur, reg_a1, args[1]);
        __attribute__((fallthrough));
    case 1:
        ins_nr += ins_insert_before_li_d(cur, reg_a0, args[0]);
        __attribute__((fallthrough));
    case 0:
        ins_nr += ins_insert_before_li_d(cur, itemp_target, (uint64_t)funptr);
        ins_insert_before(cur, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
        ins_nr++;
        break;
    default:
        lsassertm(0, "invalid arg count\n");
        break;
    }
    reg_free_itemp(itemp_target);

    /* update INS */
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    bbl->ins_head->first_ins = cur;
    bbl->ins_head->len += ins_nr;
}


VOID TRACE_InsertCall(TRACE trace, IPOINT action, AFUNPTR funptr, ...)
{

}
