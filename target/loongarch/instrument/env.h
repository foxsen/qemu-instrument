#ifndef _ENV_H_
#define _ENV_H_

#include "qemu/osdep.h"
#include "exec/exec-all.h"
/* FIXME: include above 后可以 include cpu.h */
#include "target/loongarch/cpu.h"
/* #include "qemu/typedefs.h" // seems no need */
#include "qemu.h"
#include "stddef.h"


static inline int env_offset_of_cpu_state(CPUState *cs)
{
    /* note: the result is larger than 12 bits, so we can not use ADDI_D to calc addr */
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(cs) - (uint64_t)cpu);
}

static inline int env_offset_of_gpr(CPUState *cs, int i)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->gpr[i]) - (uint64_t)cpu);
}

static inline int env_offset_of_pc(CPUState *cs)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->pc) - (uint64_t)cpu);
}

static inline int env_offset_of_fpr(CPUState *cs, int i)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->fpr[i]) - (uint64_t)cpu);
}

static inline int env_offset_of_fcc(CPUState *cs, int i)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->cf[i]) - (uint64_t)cpu);
}

static inline int env_offset_of_fscr0(CPUState *cs)
{
    /* FIXME: fcsr0_mask not considered */
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->fcsr0) - (uint64_t)cpu);
}

static inline int env_offset_of_tb_jmp_cache(CPUState *cs)
{
    /* note: the result is larger than 12 bits, so we can not use ADDI_D to calc addr */
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cs->tb_jmp_cache) - (uint64_t)cpu);
}

static inline int env_offset_of_exception_index(CPUState *cs)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cs->exception_index) - (uint64_t)cpu);
}

static inline int env_offset_of_opaque(CPUState *cs)
{
    /* ((TaskState *)cs->opaque)->ts_tid is thread id  */
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cs->opaque) - (uint64_t)cpu);
}

static inline int opaque_offset_of_ts_tid(void)
{
    return offsetof(TaskState, ts_tid);
}

/* static inline int env_offset_exception_next_eip(CPUState *cs) */
/* { */
/*     CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr; */
/*     return (int)((uint64_t)(&) - (uint64_t)cpu); */
/*     return (int)((ADDR)(&cpu->exception_next_eip) - (ADDR)lsenv->cpu_state); */
/*     /1* to be continue ... *1/ */
/* } */

static inline int env_offset_of_host_sp(CPUState *cs)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->host_sp) - (uint64_t)cpu);
}

static inline int env_offset_of_host_tp(CPUState *cs)
{
    CPULoongArchState *cpu = (CPULoongArchState *)cs->env_ptr;
    return (int)((uint64_t)(&cpu->host_tp) - (uint64_t)cpu);
}

#endif

