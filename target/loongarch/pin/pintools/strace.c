#include "pintool.h"
#include "../ins_inspection.h"


/* syscall */
static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    CPULoongArchState * env= ctxt->env;
    uint64_t syscall_nr = env->gpr[11];
    /* print_syscall(env, num, env->gpr[4], env->gpr[5], env->gpr[6], env->gpr[7], env->gpr[8], env->gpr[9]); */
    fprintf(stderr, "Thread %u: [SYSCALL] %s (a0-a6: %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n", threadIndex, syscall_name(syscall_nr), ctxt->env->gpr[4], ctxt->env->gpr[5], ctxt->env->gpr[6], ctxt->env->gpr[7], ctxt->env->gpr[8], ctxt->env->gpr[9], ctxt->env->gpr[10]);
}

static VOID syscall_exit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    /* 提前知道 syscall_nr 才能 ... */
    CPULoongArchState * env= ctxt->env;
    fprintf(stderr, "Thread %u: [SYSCALL_RET] %ld\n", threadIndex, env->gpr[4]);
}


 
static INT32 Usage(void)
{
    return -1;
}
 
int pintool_install(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    /* syscall trace */
    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddSyscallExitFunction(syscall_exit, 0);

    return 0;
}
