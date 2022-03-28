/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Definitions for loongarch board emulation.
 *
 * Copyright (C) 2021 Loongson Technology Corporation Limited
 */

#ifndef HW_LOONGARCH_H
#define HW_LOONGARCH_H

#include "target/loongarch/cpu.h"
#include "qemu-common.h"
#include "hw/boards.h"
#include "qemu/queue.h"

#define LOONGARCH_MAX_VCPUS     4

#define FEATURE_REG             0x8
#define IOCSRF_TEMP             0
#define IOCSRF_NODECNT          1
#define IOCSRF_MSI              2
#define IOCSRF_EXTIOI           3
#define IOCSRF_CSRIPI           4
#define IOCSRF_FREQCSR          5
#define IOCSRF_FREQSCALE        6
#define IOCSRF_DVFSV1           7
#define IOCSRF_GMOD             9
#define IOCSRF_VM               11

#define IOCSR_MEM_SIZE          0x428

#define VENDOR_REG              0x10
#define CPUNAME_REG             0x20
#define MISC_FUNC_REG           0x420
#define IOCSRM_EXTIOI_EN        48

#define LOONGARCH_ISA_IO_BASE   0x18000000UL
#define LOONGARCH_ISA_IO_SIZE   0x0004000

struct LoongArchMachineState {
#define FW_CFG_ADDR             0x1e020000
#define LA_BIOS_BASE            0x1c000000
#define LA_BIOS_SIZE            (4 * 1024 * 1024)

#define LA_FDT_BASE             0x1c400000
#define LA_FDT_SIZE             0x100000

/* Kernels can be configured with 64KB pages */
#define INITRD_PAGE_SIZE        (64 * KiB)
#define INITRD_BASE             0x04000000
#define COMMAND_LINE_SIZE       4096

    /*< private >*/
    MachineState parent_obj;

    MemoryRegion lowmem;
    MemoryRegion highmem;
    MemoryRegion isa_io;
    MemoryRegion bios;

    /* State for other subsystems/APIs: */
    Notifier machine_done;
    FWCfgState  *fw_cfg;
    OnOffAuto   acpi;
    char        *oem_id;
    char        *oem_table_id;
    int fdt_size;
};

#define TYPE_LOONGARCH_MACHINE  MACHINE_TYPE_NAME("virt")
OBJECT_DECLARE_SIMPLE_TYPE(LoongArchMachineState, LOONGARCH_MACHINE)

bool loongarch_is_acpi_enabled(LoongArchMachineState *lams);
void loongarch_acpi_setup(LoongArchMachineState *lams);
#endif
