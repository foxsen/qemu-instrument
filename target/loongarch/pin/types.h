#ifndef PIN_TYPES_H
#define PIN_TYPES_H

#include <stdint.h>
#include <stdio.h> // for NULL
#include "../instrument/pin_types.h"
#include "reg.h"

typedef unsigned char BOOL;

typedef void VOID;

typedef char CHAR;
typedef unsigned int UINT;
typedef int INT;
typedef double FLT64;
typedef float FLT32;
typedef unsigned int USIZE;

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef UINT64 ADDRINT;
typedef INT64 ADDRDELTA;

typedef UINT16 OPCODE;

typedef VOID (*AFUNPTR)(void);

/* const UINT32 PIN_MAX_THREADS = 8192; */
#define PIN_MAX_THREADS 8192

/* FIXME: no use, also should not define at here */
typedef uint32_t SYSCALL_STANDARD;

#endif
