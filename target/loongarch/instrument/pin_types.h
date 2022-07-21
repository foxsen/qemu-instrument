#ifndef _PIN_TYPES_H_
#define _PIN_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "decoder/ir2.h"

typedef struct pin_ins {
    uint64_t pc;
    uint32_t opcode;

    /* origin ins translated to multiple ins */
    Ins *origin_ins;
    Ins *first_ins;
    Ins *last_ins;
    int len;    /* length of la_ins list in PIN_INS */

    struct pin_ins *next;
    struct pin_ins *prev;
} *INS;

typedef struct pin_bbl {
    uint64_t pc;
    int nr_ins;
    INS ins_head;
    INS ins_tail;
    struct pin_bbl *next;
    struct pin_bbl *prev;
} *BBL;

typedef struct pin_trace {
    uint64_t pc;
    int nr_bbl;
    int nr_ins;
    BBL bbl_head;
    BBL bbl_tail;
} *TRACE;

INS INS_alloc(uint64_t pc, uint32_t opcode, Ins *la_ins);
BBL BBL_alloc(uint64_t pc);
TRACE TRACE_alloc(uint64_t pc);

void INS_set_range(INS ins, Ins *start, Ins *end, int len);

void BBL_append_ins(BBL bbl, INS ins);
void TRACE_append_bbl(TRACE trace, BBL bbl);

#endif
