#ifndef PIN_CONTEXT_H
#define PIN_CONTEXT_H

typedef struct CPUArchState CPULoongArchState;

typedef struct CONTEXT {
    CPULoongArchState *env;
} CONTEXT;

#endif
