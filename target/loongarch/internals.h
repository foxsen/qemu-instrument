/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * QEMU LoongArch CPU -- internal functions and types
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 */

#ifndef LOONGARCH_INTERNALS_H
#define LOONGARCH_INTERNALS_H

void loongarch_translate_init(void);

void loongarch_cpu_dump_state(CPUState *cpu, FILE *f, int flags);

void QEMU_NORETURN do_raise_exception(CPULoongArchState *env,
                                      uint32_t exception,
                                      uintptr_t pc);

const char *loongarch_exception_name(int32_t exception);

void restore_fp_status(CPULoongArchState *env);

#endif
