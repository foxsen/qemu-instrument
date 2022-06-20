#include "ins_instrumentation.h"
#include "ins_inspection.h"
#include "pin_state.h"
#include <stdarg.h>
#include "../instrument/error.h"
#include "../instrument/ins.h"
#include "../instrument/regs.h"

void INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val)
{
    PIN_state.ins_cb = fun;
    PIN_state.ins_cb_val = val;
}

VOID INS_InsertCall(INS ins, IPOINT action, AFUNPTR funptr, ...)
{
    lsassert(action == IPOINT_BEFORE);

    /* Parse Arguments */
    /* 最多支持8个整数类型的参数 */
    va_list valist;
    IARG_TYPE type;
    uint64_t args[8];
    int num = 0;

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
        case IARG_INST_PTR:
            args[num++] = INS_Address(ins);
            break;
        /* TODO */
        case IARG_REG_VALUE:
            args[num++] = va_arg(valist, ADDRINT);
            break;
        case IARG_INVALID:
        default:
            lsassertm(0, "Invalid IARG_TYPE %d\n", type);
            break;
        }
    }
    va_end(valist);

    /* TODO: do we need this ... */
    /* ANALYSIS_CALL *call = create_analysis_call(funptr, num, args); */

    /* Insert an analysis-call
     * set args, set target, save ra, jirl.
     */
    int itemp_target = reg_alloc_itemp();
    int ins_nr = 0;
    switch (num) {
    case 8:
        /* TODO: save the args into mem could use less ins and also save space, but ld from men maybe slow */
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a7, args[7]);
        __attribute__((fallthrough));
    case 7:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a6, args[6]);
        __attribute__((fallthrough));
    case 6:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a5, args[5]);
        __attribute__((fallthrough));
    case 5:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a4, args[4]);
        __attribute__((fallthrough));
    case 4:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a3, args[3]);
        __attribute__((fallthrough));
    case 3:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a2, args[2]);
        __attribute__((fallthrough));
    case 2:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a1, args[1]);
        __attribute__((fallthrough));
    case 1:
        ins_nr += ins_insert_before_li_d(ins->first_ins, reg_a0, args[0]);
        __attribute__((fallthrough));
    case 0:
        ins_nr += ins_insert_before_li_d(ins->first_ins, itemp_target, (uint64_t)funptr);
        ins_insert_before(ins->first_ins, ins_create_3(LISA_JIRL, reg_ra, itemp_target, 0));
        ins_nr++;
        break;
    default:
        lsassertm(0, "invalid arg count\n");
        break;
    }
    reg_free_itemp(itemp_target);

    /* update INS */
    Ins *cur = ins->first_ins;
    for (int i = 0; i < ins_nr; ++i) {
        cur = cur->prev;
    }
    ins->first_ins = cur;
    ins->nr_ins_real += ins_nr;
}

void PIN_AddFiniFunction(FINI_CALLBACK fun, VOID *val)
{
    PIN_state.fini_cb = fun;
    PIN_state.fini_cb_val = val;
}
