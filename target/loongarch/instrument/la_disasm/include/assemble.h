#ifndef __LISA_ASSEMBLE_H_
#define __LISA_ASSEMBLE_H_

#include "ir2.h"
#include "types.h"

typedef enum {
    OPD_INVALID = 0,

    OPD_CA,
    OPD_CD,
    OPD_CJ,
    OPD_CODE,
    OPD_CONDF,
    OPD_CONDH,
    OPD_CONDL,
    OPD_CSR,
    OPD_FA,
    OPD_FCSRH,
    OPD_FCSRL,
    OPD_FD,
    OPD_FJ,
    OPD_FK,
    OPD_HINTL,
    OPD_HINTS,
    OPD_I13,
    OPD_IDXS,
    OPD_IDXM,
    OPD_IDXL,
    OPD_IDXLL,
    OPD_LEVEL,
    OPD_LSBD,
    OPD_LSBW,
    OPD_MODE,
    OPD_MSBD,
    OPD_MSBW,
    OPD_OFFS,
    OPD_OFFL,
    OPD_OFFLL,
    OPD_OPCACHE,
    OPD_OPX86,
    OPD_PTR,
    OPD_RD,
    OPD_RJ,
    OPD_RK,
    OPD_SA2,
    OPD_SA3,
    OPD_SD,
    OPD_SEQ,
    OPD_SI10,
    OPD_SI11,
    OPD_SI12,
    OPD_SI14,
    OPD_SI16,
    OPD_SI20,
    OPD_SI5,
    OPD_SI8,
    OPD_SI9,
    OPD_SJ,
    OPD_UI1,
    OPD_UI12,
    OPD_UI2,
    OPD_UI3,
    OPD_UI4,
    OPD_UI5H,
    OPD_UI5L,
    OPD_UI6,
    OPD_UI7,
    OPD_UI8,
    OPD_VA,
    OPD_VD,
    OPD_VJ,
    OPD_VK,
    OPD_XA,
    OPD_XD,
    OPD_XJ,
    OPD_XK,
} LISA_OPND_TYPE;

typedef struct pair {
    int start;
    int end;
} pair;

typedef struct lisa_opnd_bit_field {
    LISA_OPND_TYPE type;
    pair bit_range_0;
    pair bit_range_1; /* some branch offset is splited into 2 parts */
} lisa_opnd_bit_field;

typedef struct lisa_insn_format {
    IR2_INS_OP op;
    uint32_t opcode;
    LISA_OPND_TYPE opnd[4];
} lisa_insn_format;


extern lisa_opnd_bit_field disa_bit_field_table[];
extern lisa_insn_format disa_lisa_format_table[];

/* uint32_t lisa_assemble(IR2 *pir2); */
uint32_t la_assemble(IR2 *pir2);

#endif
