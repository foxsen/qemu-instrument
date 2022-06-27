#include "ins_inspection.h"
#include "../instrument/ins.h"
#include "../instrument/error.h"

ADDRINT INS_Address(INS ins)
{
    return ins->pc;
}

BOOL INS_IsMemoryRead(INS ins)
{
    return op_is_load(ins->origin_ins->op);
}

USIZE INS_MemoryOperandSize(INS ins, UINT32 memoryOp)
{
    /* memoryOp is not used */
    IR2_INS_OP op = ins->origin_ins->op;
    switch (op) {
    case LISA_LD_B:
    case LISA_ST_B:
    case LISA_LD_BU:
    case LISA_VLDREPL_B:
    case LISA_VSTELM_B:
    case LISA_XVLDREPL_B:
    case LISA_XVSTELM_B:
        return 1;
    case LISA_LD_H:
    case LISA_ST_H:
    case LISA_LD_HU:
    case LISA_VLDREPL_H:
    case LISA_VSTELM_H:
    case LISA_XVLDREPL_H:
    case LISA_XVSTELM_H:
        return 2;
    case LISA_LD_W:
    case LISA_ST_W:
    case LISA_LL_W:
    case LISA_SC_W:
    case LISA_LDPTR_W:
    case LISA_STPTR_W:
    case LISA_LD_WU:
    case LISA_FLD_S:
    case LISA_FST_S:
    case LISA_VLDREPL_W:
    case LISA_VSTELM_W:
    case LISA_XVLDREPL_W:
    case LISA_XVSTELM_W:
        return 4;
    case LISA_LD_D:
    case LISA_ST_D:
    case LISA_LL_D:
    case LISA_SC_D:
    case LISA_LDPTR_D:
    case LISA_STPTR_D:
    case LISA_FLD_D:
    case LISA_FST_D:
    case LISA_VLDREPL_D:
    case LISA_VSTELM_D:
    case LISA_XVLDREPL_D:
    case LISA_XVSTELM_D:
        return 8;
    case LISA_VLD:
    case LISA_VST:
        return 16;
    case LISA_XVLD:
    case LISA_XVST:
        return 32;
    case LISA_PRELD:
    /* case LISA_LDL_W: */
    /* case LISA_LDR_W: */
    /* case LISA_LDL_D: */
    /* case LISA_LDR_D: */
    /* case LISA_STL_W: */
    /* case LISA_STR_W: */
    /* case LISA_STL_D: */
    /* case LISA_STR_D: */
    default:
            lsassertm(0, "unhandled ins op\n");
    }
}


BOOL INS_HasFallThrough(INS ins)
{
    /* TODO: does syscall & break have fallthrough? */
    IR2_INS_OP op = ins->origin_ins->op;
    if (op_is_branch(op) && !op_is_condition_jmp(op))
        return false;
    return true;
    
}

/* TRACE */
BBL TRACE_BblHead(TRACE trace)
{
    return trace->bbl_head;
}

BBL TRACE_BblTail(TRACE trace)
{
    return trace->bbl_tail;
}

UINT32 TRACE_NumBbl(TRACE trace)
{
    return trace->nr_bbl;
}

UINT32 TRACE_NumIns(TRACE trace)
{
    return trace->nr_ins;
}


/* BBL */
UINT32 BBL_NumIns(BBL bbl)
{
    return bbl->nr_ins;
}

INS BBL_InsHead(BBL x)
{
    return x->ins_head;
}

INS BBL_InsTail(BBL x)
{
    return x->ins_tail;
}

BBL BBL_Next(BBL x)
{
    return x->next;
}

BBL BBL_Prev(BBL x)
{
    return x->prev;
}

BOOL BBL_Valid(BBL x)
{
    return (x != NULL);
}
