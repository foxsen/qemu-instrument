/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * QEMU loongson 3a5000 develop board emulation
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/units.h"
#include "qemu/datadir.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/char/serial.h"
#include "sysemu/sysemu.h"
#include "sysemu/qtest.h"
#include "sysemu/runstate.h"
#include "sysemu/reset.h"
#include "sysemu/rtc.h"
#include "hw/irq.h"
#include "net/net.h"
#include "hw/loader.h"
#include "elf.h"
#include "hw/loongarch/loongarch.h"
#include "hw/intc/loongarch_ipi.h"
#include "hw/intc/loongarch_extioi.h"
#include "hw/intc/loongarch_pch_pic.h"
#include "hw/intc/loongarch_pch_msi.h"
#include "hw/pci-host/ls7a.h"
#include "hw/pci-host/gpex.h"
#include "hw/misc/unimp.h"
#include "hw/loongarch/fw_cfg.h"
#include "hw/misc/unimp.h"
#include "hw/loongarch/fw_cfg.h"
#include "hw/firmware/smbios.h"
#include "hw/acpi/aml-build.h"
#include "qapi/qapi-visit-common.h"
#include "sysemu/device_tree.h"

#include <libfdt.h>

#include "target/loongarch/cpu.h"

#define LOONGSON3_BIOSNAME "loongarch_bios.bin"

struct la_memmap_entry {
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
};

static struct la_memmap_entry *la_memmap_table;
static unsigned la_memmap_entries;

static int la_memmap_add_entry(uint64_t address, uint64_t length, uint32_t type)
{
    int i;

    for (i = 0; i < la_memmap_entries; i++) {
        if (la_memmap_table[i].address == address) {
            fprintf(stderr, "%s address:0x%lx length:0x%lx already exists\n",
                     __func__, address, length);
            return 0;
        }
    }

    la_memmap_table = g_renew(struct la_memmap_entry, la_memmap_table,
                                                      la_memmap_entries + 1);
    la_memmap_table[la_memmap_entries].address = cpu_to_le64(address);
    la_memmap_table[la_memmap_entries].length = cpu_to_le64(length);
    la_memmap_table[la_memmap_entries].type = cpu_to_le32(type);
    la_memmap_entries++;

    return la_memmap_entries;
}

static void loongarch_cpu_set_irq(void *opaque, int irq, int level)
{
    LoongArchCPU *cpu = opaque;
    CPULoongArchState *env = &cpu->env;
    CPUState *cs = CPU(cpu);

    if (irq < 0 || irq > N_IRQS) {
        return;
    }

    if (level) {
        env->CSR_ESTAT |= 1 << irq;
    } else {
        env->CSR_ESTAT &= ~(1 << irq);
    }

    if (FIELD_EX64(env->CSR_ESTAT, CSR_ESTAT, IS)) {
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    } else {
        cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
    }
}

static struct _loaderparams {
    unsigned long ram_size;
    const char *kernel_filename;
    const char *kernel_cmdline;
    const char *initrd_filename;
} loaderparams;

static uint64_t cpu_loongarch_virt_to_phys(void *opaque, uint64_t addr)
{
    return addr & 0x1fffffffll;
}

static void fw_cfg_add_kernel_info(FWCfgState *fw_cfg)
{
    int64_t kernel_entry, kernel_low, kernel_high, initrd_size = 0;
    long kernel_size;
    ram_addr_t initrd_offset = 0;
    void *cmdline_buf;
    int ret = 0;

    kernel_size = load_elf(loaderparams.kernel_filename, NULL,
                           cpu_loongarch_virt_to_phys, NULL,
                           (uint64_t *)&kernel_entry, (uint64_t *)&kernel_low,
                           (uint64_t *)&kernel_high, NULL, 0,
                           EM_LOONGARCH, 1, 0);

    if (kernel_size < 0) {
        error_report("could not load kernel '%s': %s",
                     loaderparams.kernel_filename,
                     load_elf_strerror(kernel_size));
        exit(1);
    }

    fw_cfg_add_i64(fw_cfg, FW_CFG_KERNEL_ENTRY, kernel_entry);

    if (loaderparams.initrd_filename) {
        initrd_size = get_image_size(loaderparams.initrd_filename);

        if (initrd_size > 0) {
            initrd_offset = MAX(INITRD_BASE,
                                ROUND_UP(kernel_high, INITRD_PAGE_SIZE));
            if (initrd_offset + initrd_size > 0x10000000) {
                error_report("ramdisk '%s' is too big",
                             loaderparams.initrd_filename);
                exit(1);
            }
            initrd_size = load_image_targphys(loaderparams.initrd_filename,
                                    initrd_offset,
                                    loaderparams.ram_size - initrd_offset);
        }
        if (initrd_size == (target_ulong) -1) {
            error_report("could not load initial ram disk '%s'",
                         loaderparams.initrd_filename);
            exit(1);
        }
    }

    cmdline_buf = g_malloc0(COMMAND_LINE_SIZE);
    if (initrd_size > 0)
        ret = (1 + snprintf(cmdline_buf, COMMAND_LINE_SIZE,
                "initrd=0x%lx,%li %s", initrd_offset,
                initrd_size, loaderparams.kernel_cmdline));
    else
        ret = (1 + snprintf(cmdline_buf, COMMAND_LINE_SIZE, "%s",
                loaderparams.kernel_cmdline));

    fw_cfg_add_i32(fw_cfg, FW_CFG_CMDLINE_SIZE, ret);
    fw_cfg_add_string(fw_cfg, FW_CFG_CMDLINE_DATA, (const char *)cmdline_buf);
}

static void loongarch_build_smbios(LoongArchMachineState *lams)
{
    MachineState *ms = MACHINE(lams);
    MachineClass *mc = MACHINE_GET_CLASS(lams);
    uint8_t *smbios_tables, *smbios_anchor;
    size_t smbios_tables_len, smbios_anchor_len;
    const char *product = "QEMU Virtual Machine";

    if (!lams->fw_cfg) {
        return;
    }

    product = "Loongson-3A5000-7A1000-TCG";

    smbios_set_defaults("QEMU", product, mc->name, false,
                        true, SMBIOS_ENTRY_POINT_TYPE_64);

    smbios_get_tables(ms, NULL, 0, &smbios_tables, &smbios_tables_len,
                      &smbios_anchor, &smbios_anchor_len, &error_fatal);

    if (smbios_anchor) {
        fw_cfg_add_file(lams->fw_cfg, "etc/smbios/smbios-tables",
                        smbios_tables, smbios_tables_len);
        fw_cfg_add_file(lams->fw_cfg, "etc/smbios/smbios-anchor",
                        smbios_anchor, smbios_anchor_len);
    }
}

static
void loongarch_machine_done(Notifier *notifier, void *data)
{
    LoongArchMachineState *lams = container_of(notifier,
                                        LoongArchMachineState, machine_done);
    loongarch_acpi_setup(lams);
    loongarch_build_smbios(lams);
}

static void loongarch_cpu_reset(void *opaque)
{
    LoongArchCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}

static void loongarch_qemu_write(void *opaque, hwaddr addr,
                                 uint64_t val, unsigned size)
{
}

static uint64_t loongarch_qemu_read(void *opaque, hwaddr addr, unsigned size)
{
    uint64_t feature = 0UL;

    switch (addr) {
    case FEATURE_REG:
        feature |= 1UL << IOCSRF_MSI | 1UL << IOCSRF_EXTIOI |
                   1UL << IOCSRF_CSRIPI;
        return feature ;
    case VENDOR_REG:
        return *(uint64_t *)"Loongson";
    case CPUNAME_REG:
        return *(uint64_t *)"3A5000";
    case MISC_FUNC_REG:
        return 1UL << IOCSRM_EXTIOI_EN;
    }
    return 0;
}

static const MemoryRegionOps loongarch_qemu_ops = {
    .read = loongarch_qemu_read,
    .write = loongarch_qemu_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 8,
    },
    .impl = {
        .min_access_size = 4,
        .max_access_size = 8,
    },
};

static struct DeviceState *ipi, *extioi;

static void loongarch_cpu_init(LoongArchCPU *la_cpu, int cpu_num)
{
    unsigned long ipi_addr;
    CPULoongArchState *env;
    env = &la_cpu->env;

    qdev_init_gpio_in(DEVICE(la_cpu), loongarch_cpu_set_irq, N_IRQS);

    memory_region_init_io(&env->system_iocsr, OBJECT(la_cpu), NULL,
                      env, "iocsr", UINT64_MAX);
    address_space_init(&env->address_space_iocsr, &env->system_iocsr, "IOCSR");

    timer_init_ns(&la_cpu->timer, QEMU_CLOCK_VIRTUAL,
                  &loongarch_constant_timer_cb, la_cpu);

    qemu_register_reset(loongarch_cpu_reset, la_cpu);

    memory_region_init_io(&env->iocsr_mem, OBJECT(la_cpu), &loongarch_qemu_ops,
                          NULL, "iocsr_misc", IOCSR_MEM_SIZE);

    memory_region_add_subregion(&env->system_iocsr, 0, &env->iocsr_mem);
    /* ipi memory region */
    ipi_addr = SMP_IPI_MAILBOX + cpu_num * 0x100;
    memory_region_add_subregion(&env->system_iocsr, ipi_addr,
                                sysbus_mmio_get_region(SYS_BUS_DEVICE(ipi),
                                cpu_num));
    /* extioi memory region */
    memory_region_add_subregion(&env->system_iocsr, APIC_BASE,
                                sysbus_mmio_get_region(SYS_BUS_DEVICE(extioi),
                                cpu_num));
}

static DeviceState *create_ipi(void)
{
    DeviceState *ipi;

    ipi = qdev_new(TYPE_LOONGARCH_IPI);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(ipi), &error_fatal);
    return ipi;
}

static DeviceState *create_extioi(void)
{
    DeviceState *extioi;

    extioi = qdev_new(TYPE_LOONGARCH_EXTIOI);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(extioi), &error_fatal);
    return extioi;
}

static void loongarch_devices_init(DeviceState *pch_pic)
{
    DeviceState *gpex_dev, *ls7a_pm;
    SysBusDevice *d;
    PCIBus *pci_bus;
    MemoryRegion *ecam_alias, *ecam_reg, *pio_alias, *pio_reg;
    MemoryRegion *mmio_alias, *mmio_reg, *pm_reg;
    int i;

    gpex_dev = qdev_new(TYPE_GPEX_HOST);
    d = SYS_BUS_DEVICE(gpex_dev);
    sysbus_realize_and_unref(d, &error_fatal);
    pci_bus = PCI_HOST_BRIDGE(gpex_dev)->bus;

    /* Map only part size_ecam bytes of ECAM space */
    ecam_alias = g_new0(MemoryRegion, 1);
    ecam_reg = sysbus_mmio_get_region(d, 0);
    memory_region_init_alias(ecam_alias, OBJECT(gpex_dev), "pcie-ecam",
                             ecam_reg, 0, LS_PCIECFG_SIZE);
    memory_region_add_subregion(get_system_memory(), LS_PCIECFG_BASE,
                                ecam_alias);

    /* Map PCI mem space */
    mmio_alias = g_new0(MemoryRegion, 1);
    mmio_reg = sysbus_mmio_get_region(d, 1);
    memory_region_init_alias(mmio_alias, OBJECT(gpex_dev), "pcie-mmio",
                             mmio_reg, LS7A_PCI_MEM_BASE, LS7A_PCI_MEM_SIZE);
    memory_region_add_subregion(get_system_memory(), LS7A_PCI_MEM_BASE,
                                mmio_alias);

    /* Map PCI IO port space. */
    pio_alias = g_new0(MemoryRegion, 1);
    pio_reg = sysbus_mmio_get_region(d, 2);
    memory_region_init_alias(pio_alias, OBJECT(gpex_dev), "pcie-io", pio_reg,
                             LS7A_PCI_IO_OFFSET, LS7A_PCI_IO_SIZE);
    memory_region_add_subregion(get_system_memory(), LS7A_PCI_IO_BASE,
                                pio_alias);

    for (i = 0; i < GPEX_NUM_IRQS; i++) {
        sysbus_connect_irq(d, i,
                           qdev_get_gpio_in(pch_pic, 16 + i));
        gpex_set_irq_num(GPEX_HOST(gpex_dev), i, 16 + i);
    }

    serial_mm_init(get_system_memory(), LS7A_UART_BASE, 0,
                   qdev_get_gpio_in(pch_pic,
                                    LS7A_UART_IRQ - PCH_PIC_IRQ_OFFSET),
                   115200, serial_hd(0), DEVICE_LITTLE_ENDIAN);

    /* Network init */
    for (i = 0; i < nb_nics; i++) {
        NICInfo *nd = &nd_table[i];

        if (!nd->model) {
            nd->model = g_strdup("virtio");
        }

        pci_nic_init_nofail(nd, pci_bus, nd->model, NULL);
    }

    /* VGA setup */
    pci_vga_init(pci_bus);

    /*
     * There are some invalid guest memory access.
     * Create some unimplemented devices to emulate this.
     */
    create_unimplemented_device("pci-dma-cfg", 0x1001041c, 0x4);

    sysbus_create_simple("ls7a_rtc", LS7A_RTC_REG_BASE,
                         qdev_get_gpio_in(pch_pic,
                         LS7A_RTC_IRQ - PCH_PIC_IRQ_OFFSET));

    /* Init pm */
    ls7a_pm = qdev_new(TYPE_LS7A_PM);
    d = SYS_BUS_DEVICE(ls7a_pm);
    sysbus_realize_and_unref(d, &error_fatal);
    ls7a_pm_init(ls7a_pm, qdev_get_gpio_in(pch_pic,
                                           ACPI_SCI_IRQ - PCH_PIC_IRQ_OFFSET));
    pm_reg = sysbus_mmio_get_region(d, 0);
    memory_region_add_subregion(get_system_memory(), ACPI_IO_BASE, pm_reg);
    memory_region_add_subregion(pm_reg, LS7A_GPE0_STS_REG,
                                sysbus_mmio_get_region(d, 1));
    memory_region_add_subregion(pm_reg, LS7A_GPE0_RESET_REG,
                                sysbus_mmio_get_region(d, 2));
}

static void loongarch_irq_init(LoongArchMachineState *lams,
                               DeviceState *ipi, DeviceState *extioi)
{
    MachineState *ms = MACHINE(lams);
    DeviceState *pch_pic, *pch_msi, *cpudev;

    SysBusDevice *d;
    int cpu, pin, i;

    for (cpu = 0; cpu < ms->smp.cpus; cpu++) {
        cpudev = DEVICE(qemu_get_cpu(cpu));
        /* connect ipi irq to cpu irq */
        qdev_connect_gpio_out(ipi, cpu, qdev_get_gpio_in(cpudev, IRQ_IPI));
    }

    for (i = 0; i < EXTIOI_IRQS; i++) {
        sysbus_connect_irq(SYS_BUS_DEVICE(extioi),
                           i, qdev_get_gpio_in(extioi, i));
    }

    /*
     * connect ext irq to the cpu irq
     * cpu_pin[9:2] <= intc_pin[7:0]
     */
    for (cpu = 0; cpu < ms->smp.cpus; cpu++) {
        cpudev = DEVICE(qemu_get_cpu(cpu));
        for (pin = 0; pin < LS3A_INTC_IP; pin++) {
            qdev_connect_gpio_out(extioi, (cpu * 8 + pin),
                                  qdev_get_gpio_in(cpudev, pin + 2));
        }
    }

    pch_pic = qdev_new(TYPE_LOONGARCH_PCH_PIC);
    d = SYS_BUS_DEVICE(pch_pic);
    sysbus_realize_and_unref(d, &error_fatal);
    memory_region_add_subregion(get_system_memory(), LS7A_IOAPIC_REG_BASE,
                            sysbus_mmio_get_region(d, 0));
    memory_region_add_subregion(get_system_memory(),
                            LS7A_IOAPIC_REG_BASE + PCH_PIC_ROUTE_ENTRY_OFFSET,
                            sysbus_mmio_get_region(d, 1));
    memory_region_add_subregion(get_system_memory(),
                            LS7A_IOAPIC_REG_BASE + PCH_PIC_INT_STATUS_LO,
                            sysbus_mmio_get_region(d, 2));

    /* Connect 64 pch_pic irqs to extioi */
    for (int i = 0; i < PCH_PIC_IRQ_NUM; i++) {
        sysbus_connect_irq(d, i, qdev_get_gpio_in(extioi, i));
    }

    pch_msi = qdev_new(TYPE_LOONGARCH_PCH_MSI);
    d = SYS_BUS_DEVICE(pch_msi);
    sysbus_realize_and_unref(d, &error_fatal);
    sysbus_mmio_map(d, 0, LS7A_PCH_MSI_ADDR_LOW);
    for (i = 0; i < PCH_MSI_IRQ_NUM; i++) {
        /* Connect 192 pch_msi irqs to extioi */
        sysbus_connect_irq(d, i,
                           qdev_get_gpio_in(extioi, i + PCH_MSI_IRQ_START));
    }

    loongarch_devices_init(pch_pic);
}

static void create_fdt(LoongArchMachineState *lams)
{
    MachineState *ms = MACHINE(lams);

    ms->fdt = create_device_tree(&lams->fdt_size);
    if (!ms->fdt) {
        error_report("create_device_tree() failed");
        exit(1);
    }

    /* Header */
    qemu_fdt_setprop_string(ms->fdt, "/", "compatible",
                            "linux,dummy-loongson3");
    qemu_fdt_setprop_cell(ms->fdt, "/", "#address-cells", 0x2);
    qemu_fdt_setprop_cell(ms->fdt, "/", "#size-cells", 0x2);
}

static void fdt_add_cpu_nodes(const LoongArchMachineState *lams)
{
    int num;
    const MachineState *ms = MACHINE(lams);
    int smp_cpus = ms->smp.cpus;

    qemu_fdt_add_subnode(ms->fdt, "/cpus");
    qemu_fdt_setprop_cell(ms->fdt, "/cpus", "#address-cells", 0x1);
    qemu_fdt_setprop_cell(ms->fdt, "/cpus", "#size-cells", 0x0);

    /* cpu nodes */
    for (num = smp_cpus - 1; num >= 0; num--) {
        char *nodename = g_strdup_printf("/cpus/cpu@%d", num);
        LoongArchCPU *cpu = LOONGARCH_CPU(qemu_get_cpu(num));

        qemu_fdt_add_subnode(ms->fdt, nodename);
        qemu_fdt_setprop_string(ms->fdt, nodename, "device_type", "cpu");
        qemu_fdt_setprop_string(ms->fdt, nodename, "compatible",
                                cpu->dtb_compatible);
        qemu_fdt_setprop_cell(ms->fdt, nodename, "reg", num);
        qemu_fdt_setprop_cell(ms->fdt, nodename, "phandle",
                              qemu_fdt_alloc_phandle(ms->fdt));
        g_free(nodename);
    }

    /*cpu map */
    qemu_fdt_add_subnode(ms->fdt, "/cpus/cpu-map");

    for (num = smp_cpus - 1; num >= 0; num--) {
        char *cpu_path = g_strdup_printf("/cpus/cpu@%d", num);
        char *map_path;

        if (ms->smp.threads > 1) {
            map_path = g_strdup_printf(
                "/cpus/cpu-map/socket%d/core%d/thread%d",
                num / (ms->smp.cores * ms->smp.threads),
                (num / ms->smp.threads) % ms->smp.cores,
                num % ms->smp.threads);
        } else {
            map_path = g_strdup_printf(
                "/cpus/cpu-map/socket%d/core%d",
                num / ms->smp.cores,
                num % ms->smp.cores);
        }
        qemu_fdt_add_path(ms->fdt, map_path);
        qemu_fdt_setprop_phandle(ms->fdt, map_path, "cpu", cpu_path);

        g_free(map_path);
        g_free(cpu_path);
    }
}

static void fdt_add_fw_cfg_node(const LoongArchMachineState *lams)
{
    char *nodename;
    hwaddr base = FW_CFG_ADDR;
    const MachineState *ms = MACHINE(lams);

    nodename = g_strdup_printf("/fw_cfg@%" PRIx64, base);
    qemu_fdt_add_subnode(ms->fdt, nodename);
    qemu_fdt_setprop_string(ms->fdt, nodename,
                            "compatible", "qemu,fw-cfg-mmio");
    qemu_fdt_setprop_sized_cells(ms->fdt, nodename, "reg",
                                 2, base, 2, 0x8);
    qemu_fdt_setprop(ms->fdt, nodename, "dma-coherent", NULL, 0);
    g_free(nodename);
}

static void fdt_add_pcie_node(const LoongArchMachineState *lams)
{
    char *nodename;
    hwaddr base_mmio = LS7A_PCI_MEM_BASE;
    hwaddr size_mmio = LS7A_PCI_MEM_SIZE;
    hwaddr base_pio = LS7A_PCI_IO_BASE;
    hwaddr size_pio = LS7A_PCI_IO_SIZE;
    hwaddr base_pcie = LS_PCIECFG_BASE;
    hwaddr size_pcie = LS_PCIECFG_SIZE;
    hwaddr base = base_pcie;

    const MachineState *ms = MACHINE(lams);

    nodename = g_strdup_printf("/pcie@%" PRIx64, base);
    qemu_fdt_add_subnode(ms->fdt, nodename);
    qemu_fdt_setprop_string(ms->fdt, nodename,
                            "compatible", "pci-host-ecam-generic");
    qemu_fdt_setprop_string(ms->fdt, nodename, "device_type", "pci");
    qemu_fdt_setprop_cell(ms->fdt, nodename, "#address-cells", 3);
    qemu_fdt_setprop_cell(ms->fdt, nodename, "#size-cells", 2);
    qemu_fdt_setprop_cell(ms->fdt, nodename, "linux,pci-domain", 0);
    qemu_fdt_setprop_cells(ms->fdt, nodename, "bus-range", 0,
                           PCIE_MMCFG_BUS(LS_PCIECFG_SIZE - 1));
    qemu_fdt_setprop(ms->fdt, nodename, "dma-coherent", NULL, 0);
    qemu_fdt_setprop_sized_cells(ms->fdt, nodename, "reg",
                                 2, base_pcie, 2, size_pcie);
    qemu_fdt_setprop_sized_cells(ms->fdt, nodename, "ranges",
                                 1, FDT_PCI_RANGE_IOPORT, 2, LS7A_PCI_IO_OFFSET,
                                 2, base_pio, 2, size_pio,
                                 1, FDT_PCI_RANGE_MMIO, 2, base_mmio,
                                 2, base_mmio, 2, size_mmio);
    g_free(nodename);
    qemu_fdt_dumpdtb(ms->fdt, lams->fdt_size);
}

static void loongarch_init(MachineState *machine)
{
    const char *cpu_model = machine->cpu_type;
    const char *kernel_filename = machine->kernel_filename;
    const char *kernel_cmdline = machine->kernel_cmdline;
    const char *initrd_filename = machine->initrd_filename;
    LoongArchCPU *la_cpu;
    ipi = create_ipi();
    extioi = create_extioi();
    ram_addr_t offset = 0;
    ram_addr_t ram_size = machine->ram_size;
    uint64_t highram_size = 0;
    MemoryRegion *address_space_mem = get_system_memory();
    LoongArchMachineState *lams = LOONGARCH_MACHINE(machine);
    int i;
    int bios_size;
    char *filename;

    if (!cpu_model) {
        cpu_model = LOONGARCH_CPU_TYPE_NAME("Loongson-3A5000");
    }

    if (!strstr(cpu_model, "Loongson-3A5000")) {
        error_report("LoongArch/TCG needs cpu type Loongson-3A5000");
        exit(1);
    }

    create_fdt(lams);

    /* Init CPUs */
    for (i = 0; i < machine->smp.cpus; i++) {
        la_cpu = LOONGARCH_CPU(cpu_create(machine->cpu_type));
        loongarch_cpu_init(la_cpu, i);
    }

    fdt_add_cpu_nodes(lams);

    if (ram_size < 1 * GiB) {
        error_report("ram_size must be greater than 1G.");
        exit(1);
    }

    /* Add memory region */
    memory_region_init_alias(&lams->lowmem, NULL, "loongarch.lowram",
                             machine->ram, 0, 256 * MiB);
    memory_region_add_subregion(address_space_mem, offset, &lams->lowmem);
    offset += 256 * MiB;
    la_memmap_add_entry(0, 256 * MiB, 1);
    highram_size = ram_size - 256 * MiB;
    memory_region_init_alias(&lams->highmem, NULL, "loongarch.highmem",
                             machine->ram, offset, highram_size);
    memory_region_add_subregion(address_space_mem, 0x90000000, &lams->highmem);
    offset += highram_size;
    la_memmap_add_entry(0x90000000, highram_size, 1);
    /* Add isa io region */
    memory_region_init_alias(&lams->isa_io, NULL, "isa-io",
                             get_system_io(), 0, LOONGARCH_ISA_IO_SIZE);
    memory_region_add_subregion(address_space_mem, LOONGARCH_ISA_IO_BASE,
                                &lams->isa_io);
    /* load the BIOS image. */
    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS,
                              machine->firmware ?: LOONGSON3_BIOSNAME);
    if (filename) {
        bios_size = load_image_targphys(filename, LA_BIOS_BASE, LA_BIOS_SIZE);
        lams->fw_cfg = loongarch_fw_cfg_init(ram_size, machine);
        rom_set_fw(lams->fw_cfg);
        g_free(filename);
    } else {
        bios_size = -1;
    }

    if ((bios_size < 0 || bios_size > LA_BIOS_SIZE) && !qtest_enabled()) {
        error_report("Could not load LOONGARCH bios '%s'", machine->firmware);
        exit(1);
    }

    fdt_add_fw_cfg_node(lams);

    if (kernel_filename) {
        loaderparams.ram_size = ram_size;
        loaderparams.kernel_filename = kernel_filename;
        loaderparams.kernel_cmdline = kernel_cmdline;
        loaderparams.initrd_filename = initrd_filename;
        fw_cfg_add_kernel_info(lams->fw_cfg);
    }

    memory_region_init_ram(&lams->bios, NULL, "loongarch.bios",
                           LA_BIOS_SIZE, &error_fatal);
    memory_region_set_readonly(&lams->bios, true);
    memory_region_add_subregion(get_system_memory(), LA_BIOS_BASE, &lams->bios);
    if (lams->fw_cfg != NULL) {
        fw_cfg_add_file(lams->fw_cfg, "etc/memmap",
                        la_memmap_table,
                        sizeof(struct la_memmap_entry) * (la_memmap_entries));
    }

    /* Initialize the IO interrupt subsystem */
    loongarch_irq_init(lams, ipi, extioi);

    lams->machine_done.notify = loongarch_machine_done;
    qemu_add_machine_init_done_notifier(&lams->machine_done);

    fdt_add_pcie_node(lams);

    /* load fdt */
    MemoryRegion *fdt_rom = g_new(MemoryRegion, 1);
    memory_region_init_rom(fdt_rom, NULL, "fdt", LA_FDT_SIZE, &error_fatal);
    memory_region_add_subregion(get_system_memory(), LA_FDT_BASE, fdt_rom);
    rom_add_blob_fixed("fdt", machine->fdt, lams->fdt_size, LA_FDT_BASE);
}

bool loongarch_is_acpi_enabled(LoongArchMachineState *lams)
{
    if (lams->acpi == ON_OFF_AUTO_OFF) {
        return false;
    }
    return true;
}

static void loongarch_get_acpi(Object *obj, Visitor *v, const char *name,
                               void *opaque, Error **errp)
{
    LoongArchMachineState *lams = LOONGARCH_MACHINE(obj);
    OnOffAuto acpi = lams->acpi;

    visit_type_OnOffAuto(v, name, &acpi, errp);
}

static void loongarch_set_acpi(Object *obj, Visitor *v, const char *name,
                               void *opaque, Error **errp)
{
    LoongArchMachineState *lams = LOONGARCH_MACHINE(obj);

    visit_type_OnOffAuto(v, name, &lams->acpi, errp);
}

static void loongarch_machine_initfn(Object *obj)
{
    LoongArchMachineState *lams = LOONGARCH_MACHINE(obj);

    lams->acpi = ON_OFF_AUTO_AUTO;
    lams->oem_id = g_strndup(ACPI_BUILD_APPNAME6, 6);
    lams->oem_table_id = g_strndup(ACPI_BUILD_APPNAME8, 8);
}

static void loongarch_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "Loongson-3A5000 LS7A1000 machine";
    mc->init = loongarch_init;
    mc->default_ram_size = 1 * GiB;
    mc->default_cpu_type = LOONGARCH_CPU_TYPE_NAME("Loongson-3A5000");
    mc->default_ram_id = "loongarch.ram";
    mc->max_cpus = LOONGARCH_MAX_VCPUS;
    mc->is_default = 1;
    mc->default_machine_opts = "firmware=loongarch_bios.bin";
    mc->default_kernel_irqchip_split = false;
    mc->block_default_type = IF_VIRTIO;
    mc->default_boot_order = "c";
    mc->no_cdrom = 1;

    object_class_property_add(oc, "acpi", "OnOffAuto",
        loongarch_get_acpi, loongarch_set_acpi,
        NULL, NULL);
    object_class_property_set_description(oc, "acpi",
        "Enable ACPI");
}

static const TypeInfo loongarch_machine_types[] = {
    {
        .name           = TYPE_LOONGARCH_MACHINE,
        .parent         = TYPE_MACHINE,
        .instance_size  = sizeof(LoongArchMachineState),
        .instance_init = loongarch_machine_initfn,
        .class_init     = loongarch_class_init,
    }
};

DEFINE_TYPES(loongarch_machine_types)
