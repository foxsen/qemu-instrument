#include "thread.h"
#include "qemu/osdep.h"
#include "qemu.h"

uint32_t pin_thread_id_count = 0;
__thread uint32_t pin_thread_id = 0;

void PIN_thread_create(void)
{
    pin_thread_id = ++pin_thread_id_count;
}

THREADID PIN_ThreadId(void)
{
    return ((TaskState *)current_cpu->opaque)->ts_tid;
}
