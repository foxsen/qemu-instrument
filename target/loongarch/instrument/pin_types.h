#ifndef _PIN_TYPES_H_
#define _PIN_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "decoder/ir2.h"

typedef struct pin_ins {
    uint64_t pc;
    uint32_t opcode;
    Ins *origin_ins;

    /* origin_ins is translated to an ins linked list */
    Ins *first_ins;
    Ins *last_ins;
    int len;

    struct pin_ins *next;
    struct pin_ins *prev;
} *INS;

typedef struct pin_bbl {
    uint64_t pc;
    int nr_ins;     // INS number in BBl
    INS ins_head;
    INS ins_tail;
    struct pin_bbl *next;
    struct pin_bbl *prev;
} *BBL;

typedef struct pin_trace {
    uint64_t pc;
    int nr_bbl;
    int nr_ins;     // INS number in TRACE
    BBL bbl_head;
    BBL bbl_tail;
} *TRACE;

/* the real type of IMG is defined in elf/symbol.cpp named struct image */
typedef void *IMG;

typedef struct pin_rtn {
    const char *name;
    uint64_t addr;
    uint64_t size;
} *RTN;

INS INS_alloc(uint64_t pc, uint32_t opcode, Ins *origin_ins);
BBL BBL_alloc(uint64_t pc);
TRACE TRACE_alloc(uint64_t pc);

void BBL_append_ins(BBL bbl, INS ins);
void TRACE_append_bbl(TRACE trace, BBL bbl);

#ifdef __cplusplus
extern "C" {
#endif
RTN RTN_alloc(const char *name, uint64_t addr, uint64_t size);
#ifdef __cplusplus
}
#endif

#endif
