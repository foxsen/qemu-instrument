#include "pintool.h"
#include "../../instrument/decoder/disasm.h"
#include "../../instrument/decoder/la_print.h"
#include "../ins_inspection.h"
/* #include "qemu.h" */
/* #include "user-internals.h" */
/* #include "strace.h" */
#include "../../instrument/elf/elf_parser.h"
#include <fcntl.h>

static int need_check = 0;
static int fd;

/* syscall */
static VOID syscall_enter(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    CPULoongArchState * env = ctxt->env;
    uint64_t syscall_nr = PIN_GetSyscallNumber(ctxt, std);
    
    fd = env->gpr[8];
    int prot = env->gpr[6];

    /* only trace mmap a EXEC segment */
    /* void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset); */
    if (syscall_nr != 222 || (prot & PROT_EXEC) == 0) {
        return;
    }
    /* print_syscall(env, syscall_nr, env->gpr[4], env->gpr[5], env->gpr[6], env->gpr[7], env->gpr[8], env->gpr[9]); */
    printf("Thread %u: SYSCALL: %s (a0-a6: %lx, %lx, %lx, %lx, %lx, %lx, %lx)\n", threadIndex, syscall_name(syscall_nr), ctxt->env->gpr[4], ctxt->env->gpr[5], ctxt->env->gpr[6], ctxt->env->gpr[7], ctxt->env->gpr[8], ctxt->env->gpr[9], ctxt->env->gpr[10]);

    /* TODO check MMAP_FIXED */

    need_check = 1;
}



static VOID syscall_exit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    if (!need_check) {
        return;
    }
    need_check = 0;
    CPULoongArchState * env= ctxt->env;
    uint64_t retval = env->gpr[4];
    printf("\nThread %u: SYSCALL_RET: %lx\n", threadIndex, env->gpr[4]);
    IMG img;
    parse_elf_symbol_with_fd(fd, retval, &img);
    IMG_instrument(img);
}



static VOID Init(VOID)
{
}

#include "target/loongarch/instrument/elf/symbol.h"
static VOID Fini(INT32 code, VOID* v)
{
    print_collected_symbols();
}
 
static INT32 Usage(void)
{
    return -1;
}
 
int pintool_install(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
 
    Init();


    /* syscall trace */
    PIN_AddSyscallEntryFunction(syscall_enter, 0);
    PIN_AddSyscallExitFunction(syscall_exit, 0);


    /* result */
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}
