#include "thread.h"

uint32_t pin_thread_id_count = 0;
__thread uint32_t pin_thread_id = 0;

void PIN_Thread_create(void)
{
    pin_thread_id = ++pin_thread_id_count;
}

THREADID PIN_ThreadId(void)
{
    return pin_thread_id;
}
