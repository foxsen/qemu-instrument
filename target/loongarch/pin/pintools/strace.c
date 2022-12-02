#include "pintool.h"
#include "../ins_inspection.h"


/* syscall */
static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctx, SYSCALL_STANDARD std, VOID *v)
{
    /* QEMU API: print_syscall(env, num, env->gpr[4], env->gpr[5], env->gpr[6], env->gpr[7], env->gpr[8], env->gpr[9]); */
    int syscall_nr = PIN_GetSyscallNumber(ctx, std);
    fprintf(stderr, "[tid %u] %s (a0-a6: 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)\n",
            threadIndex,
            syscall_name(syscall_nr), 
            PIN_GetSyscallArgument(ctx, std, 0),
            PIN_GetSyscallArgument(ctx, std, 1),
            PIN_GetSyscallArgument(ctx, std, 2),
            PIN_GetSyscallArgument(ctx, std, 3),
            PIN_GetSyscallArgument(ctx, std, 4),
            PIN_GetSyscallArgument(ctx, std, 5),
            PIN_GetSyscallArgument(ctx, std, 6));
}

static VOID syscall_exit(THREADID threadIndex, CONTEXT *ctx, SYSCALL_STANDARD std, VOID *v)
{
    fprintf(stderr, "[tid %u] = %ld\n",
            threadIndex, PIN_GetSyscallReturn(ctx, std));
}


 
static INT32 Usage(void)
{
    return -1;
}
 
int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    /* syscall trace */
    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddSyscallExitFunction(syscall_exit, 0);

    return 0;
}
