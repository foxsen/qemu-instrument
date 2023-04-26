#include "pin_types.h"
#include "util/error.h"
#include <stdlib.h>

INS INS_alloc(uint64_t pc, uint32_t opcode, Ins *origin_ins)
{
    /* FIXME: never free, memory leakage */
    INS INS = malloc(sizeof(struct pin_ins));
    INS->next = NULL;
    INS->prev = NULL;
    INS->pc = pc;
    INS->opcode = opcode;
    INS->origin_ins = origin_ins;
    INS->first_ins = NULL;
    INS->last_ins = NULL;
    INS->len = 0;
    INS->ibefore_next_cb = NULL;
    INS->iafter_next_cb = NULL;
    return INS;
}

BBL BBL_alloc(uint64_t pc)
{
    BBL bbl = malloc(sizeof(struct pin_bbl));
    bbl->pc = pc;
    bbl->nr_ins = 0;
    bbl->ins_head = NULL;
    bbl->ins_tail = NULL;
    bbl->next = NULL;
    bbl->prev = NULL;
    return bbl;
}

TRACE TRACE_alloc(uint64_t pc)
{
    TRACE trace = malloc(sizeof(struct pin_trace));
    trace->pc = pc;
    trace->nr_bbl = 0;
    trace->nr_ins = 0;
    trace->bbl_head = NULL;
    trace->bbl_tail = NULL;
    return trace;
}

void BBL_append_INS(BBL bbl, INS INS)
{
    bbl->nr_ins++;
    if (bbl->ins_head == NULL) {
        lsassert(bbl->ins_tail == NULL);
        bbl->ins_head = INS;
        bbl->ins_tail = INS;
    } else {
        lsassert(bbl->ins_tail != NULL);
        INS->prev = bbl->ins_tail;
        bbl->ins_tail->next = INS;
        bbl->ins_tail = INS;

        /* 连接INS之间的la_ins链表 */
        INS->first_ins->prev = INS->prev->last_ins;
        INS->prev->last_ins->next = INS->first_ins;
    }
}

void TRACE_append_BBL(TRACE trace, BBL bbl)
{
    trace->nr_bbl++;
    trace->nr_ins += bbl->nr_ins;
    if (trace->bbl_head == NULL) {
        lsassert(trace->bbl_tail == NULL);
        trace->bbl_head = bbl;
        trace->bbl_tail = bbl;
    } else {
        lsassert(trace->bbl_tail != NULL);
        bbl->prev = trace->bbl_tail;
        trace->bbl_tail->next = bbl;
        trace->bbl_tail = bbl;

        /* 连接BBL之间的la_ins链表 */
        bbl->ins_head->first_ins->prev = bbl->prev->ins_tail->last_ins;
        bbl->prev->ins_tail->last_ins->next = bbl->ins_head->first_ins;
    }
}

RTN RTN_alloc(const char *name, uint64_t addr, uint64_t size)
{
    return (RTN) {
        .name = name,
        .addr = addr,
        .size = size
    };
}

