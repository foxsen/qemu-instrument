#ifndef PIN_THREAD_H
#define PIN_THREAD_H

#include "types.h"

typedef UINT32 THREADID;

/* 仅内部使用 */
void PIN_Thread_create(void);

THREADID PIN_ThreadId(void);

#endif
