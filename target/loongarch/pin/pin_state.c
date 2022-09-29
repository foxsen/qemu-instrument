#include "pin_state.h"

PIN_STATE PIN_state = {
    .read_symbol = false,
    .ins_cb = NULL,
    .ins_cb_val = NULL,
    .trace_cb = NULL,
    .trace_cb_val = NULL,
    .fini_cb = NULL,
    .fini_cb_val = NULL,
    .syscall_entry_cb = NULL,
    .syscall_entry_cb_val = NULL,
    .syscall_exit_cb = NULL,
    .syscall_exit_cb_val = NULL,
    .cpu_exec_enter_cb = NULL,
    .cpu_exec_enter_cb_val = NULL,
    .cpu_exec_exit_cb = NULL,
    .cpu_exec_exit_cb_val = NULL,
    .img_cb = NULL,
    .img_cb_val = NULL,
};

BOOL PIN_Init(INT32 argc, CHAR** argv)
{
    return 0;
}

VOID PIN_InitSymbols(void)
{
    PIN_state.read_symbol = true;
}

void INS_instrument(INS ins)
{
    if (PIN_state.ins_cb != NULL) {
        PIN_state.ins_cb(ins, PIN_state.ins_cb_val);
    }
}

void TRACE_instrument(TRACE trace)
{
    if (PIN_state.trace_cb != NULL) {
        PIN_state.trace_cb(trace, PIN_state.trace_cb_val);
    }
    /* 检查是否需要函数插桩 */
    if (PIN_state.read_symbol) {
        RTN_instrument(trace);
    }
}

void pin_instrument_exit(CPUArchState *env, int code)
{
    if (PIN_state.fini_cb != NULL) {
        PIN_state.fini_cb(code, PIN_state.fini_cb_val);
    }
}

static int need_check_load_so = 0;
static int fd;
static void pin_instrument_load_shared_object_check(CPULoongArchState * env)
{
    /* void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset); */
    uint64_t syscall_nr = env->gpr[11];
    int prot = env->gpr[6];
    fd = env->gpr[8];

    /* Only trace mmap a EXEC segment */
    if (syscall_nr != 222 || (prot & PROT_EXEC) == 0) {
        return;
    }
    need_check_load_so = 1;
}

#include "target/loongarch/instrument/elf/elf_parser.h"
static void pin_instrument_load_shared_object(CPULoongArchState * env)
{
    if (!need_check_load_so) {
        return;
    }
    need_check_load_so = 0;

    uint64_t retval = env->gpr[4];
    IMG img;
    parse_elf_symbol_with_fd(fd, retval, &img);
    IMG_instrument(img);
}

void pin_instrument_syscall(CPUState *cpu)
{
    if (PIN_state.read_symbol) {
        pin_instrument_load_shared_object_check(cpu->env_ptr);
    }

    CONTEXT ctxt;
    ctxt.env = cpu->env_ptr;
    if (PIN_state.syscall_entry_cb != NULL) {
        /* FIXME: some args are temporarily filled with NULL and 0 */
        PIN_state.syscall_entry_cb(PIN_ThreadId(), &ctxt, 0, PIN_state.syscall_entry_cb_val);
    }
}

void pin_instrument_syscall_ret(CPUState *cpu, int num, abi_long ret)
{
    if (PIN_state.read_symbol) {
        pin_instrument_load_shared_object(cpu->env_ptr);
    }

    CONTEXT ctxt;
    ctxt.env = cpu->env_ptr;
    if (PIN_state.syscall_exit_cb != NULL) {
        /* FIXME: some args are temporarily filled with NULL and 0 */
        PIN_state.syscall_exit_cb(PIN_ThreadId(), &ctxt, 0, PIN_state.syscall_exit_cb_val);
    }
}

void pin_instrument_cpu_exec_enter(CPUState *cpu, TranslationBlock *tb)
{
    if (PIN_state.cpu_exec_enter_cb) {
        PIN_state.cpu_exec_enter_cb(cpu, tb);
    }
}

void pin_instrument_cpu_exec_exit(CPUState *cpu, TranslationBlock *last_tb, int tb_exit)
{
    /* FIEXME: when exec_tb exit for syscall/break, this cb will not be called */
    if (PIN_state.cpu_exec_exit_cb) {
        PIN_state.cpu_exec_exit_cb(cpu, last_tb, tb_exit);
    }
}

void IMG_instrument(IMG img)
{
    if (PIN_state.img_cb) {
        PIN_state.img_cb(img, PIN_state.img_cb_val);
    }
}

