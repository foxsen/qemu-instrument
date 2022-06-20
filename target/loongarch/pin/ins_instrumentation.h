#ifndef INS_TRUMENTATION_H
#define INS_TRUMENTATION_H

#include "types_foundation.h"
#include "types_base.h"
#include "instrumentation_arguements.h"
#include "../instrument/pin_types.h"

typedef VOID (*INS_INSTRUMENT_CALLBACK)(INS ins, VOID* v);

/* FIXME: change the return type to void because I dont know the class of PIN_CALLBACK */
/* PIN_CALLBACK INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val); */
void INS_AddInstrumentFunction(INS_INSTRUMENT_CALLBACK fun, VOID* val);
VOID INS_InsertCall(INS ins, IPOINT action, AFUNPTR funptr, ...);


typedef VOID(* FINI_CALLBACK) (INT32 code, VOID *v);
void PIN_AddFiniFunction(FINI_CALLBACK fun, VOID *val); 

#endif
