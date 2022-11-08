#ifndef PIN_THREAD_H
#define PIN_THREAD_H

#include "types.h"

typedef UINT32 THREADID;

#ifdef __cplusplus
extern "C" {
#endif
THREADID PIN_ThreadId(void);
#ifdef __cplusplus
}
#endif


/* 仅内部使用 */
void PIN_thread_create(void);


#endif
