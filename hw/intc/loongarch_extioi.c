/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Loongson 3A5000 ext interrupt controller emulation
 *
 * Copyright (C) 2021 Loongson Technology Corporation Limited
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/log.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/loongarch/loongarch.h"
#include "hw/qdev-properties.h"
#include "exec/address-spaces.h"
#include "hw/intc/loongarch_extioi.h"
#include "migration/vmstate.h"
#include "trace.h"

static void extioi_update_irq(void *opaque, int irq_num, int level)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    uint8_t  ipnum, cpu;
    unsigned long found1, found2;

    ipnum = s->sw_ipmap[irq_num];
    cpu   = s->sw_coremap[irq_num];
    if (level == 1) {
        if (test_bit(irq_num, (void *)s->enable) == false) {
            return;
        }
        bitmap_set((void *)s->coreisr[cpu], irq_num, 1);
        found1 = find_next_bit((void *)&(s->sw_ipisr[cpu][ipnum]),
                               EXTIOI_IRQS, 0);
        bitmap_set((void *)&(s->sw_ipisr[cpu][ipnum]), irq_num, 1);

        if (found1 >= EXTIOI_IRQS) {
            qemu_set_irq(s->parent_irq[cpu][ipnum], level);
        }
    } else {
        bitmap_clear((void *)s->coreisr[cpu], irq_num, 1);
        found1 = find_next_bit((void *)&(s->sw_ipisr[cpu][ipnum]),
                               EXTIOI_IRQS, 0);
        bitmap_clear((void *)&(s->sw_ipisr[cpu][ipnum]), irq_num, 1);
        found2 = find_next_bit((void *)&(s->sw_ipisr[cpu][ipnum]),
                               EXTIOI_IRQS, 0);

        if ((found1 < EXTIOI_IRQS) && (found2 >= EXTIOI_IRQS)) {
            qemu_set_irq(s->parent_irq[cpu][ipnum], level);
        }
    }
}

static void extioi_setirq(void *opaque, int irq, int level)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    trace_extioi_setirq(irq, level);
    extioi_update_irq(s, irq, level);
}

static uint64_t extioi_nodetype_readw(void *opaque, hwaddr addr, unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    unsigned long offset = addr & 0xffff;
    uint32_t ret, index;

    switch (offset) {
    case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END - 1:
        index = (offset - EXTIOI_NODETYPE_START) >> 2;
        ret = s->nodetype[index];
        break;
    default:
        break;
    }

    trace_loongarch_extioi_nodetype_readw((uint32_t)addr, ret);
    return ret;
}

static void extioi_nodetype_writew(void *opaque, hwaddr addr,
                                   uint64_t val, unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    int index;
    uint32_t offset;
    trace_loongarch_extioi_nodetype_writew(size, (uint32_t)addr, val);

    offset = addr & 0xffff;

    switch (offset) {
    case EXTIOI_NODETYPE_START ... EXTIOI_NODETYPE_END - 1:
        index = (offset - EXTIOI_NODETYPE_START) >> 2;
        s->nodetype[index] = val;
        break;
    default:
        break;
    }
}

static uint64_t extioi_ipmap_enable_read(void *opaque, hwaddr addr,
                                         unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    uint8_t ret;

    switch (addr) {
    case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END - 1:
        ret = s->ipmap[addr];
        break;
    case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END - 1:
        addr -= EXTIOI_ENABLE_START;
        ret = s->enable[addr];
        break;
    default:
        break;
    }

    trace_loongarch_extioi_ipmap_enable_read((uint8_t)addr, ret);
    return ret;
}

static void extioi_ipmap_enable_write(void *opaque, hwaddr addr,
                                      uint64_t value, unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    uint8_t old_data, val = value & 0xff;
    int i, level, ipnum, irqnum;
    trace_loongarch_extioi_ipmap_enable_write(size, (uint8_t)addr, val);

    switch (addr) {
    case EXTIOI_IPMAP_START ... EXTIOI_IPMAP_END - 1:
        s->ipmap[addr] = val;
        /* Routing in groups of 32 interrupt */
        ipnum = find_first_bit((void *)&val, 4);
        for (i = 0; i < 32; i++) {
            irqnum = addr * 32 + i;
            if (ipnum != 4) {
                s->sw_ipmap[irqnum] = ipnum;
            } else {
                s->sw_ipmap[irqnum] = 0;
            }
        }
        break;
    case EXTIOI_ENABLE_START ... EXTIOI_ENABLE_END - 1:
        addr -= EXTIOI_ENABLE_START;
        old_data = s->enable[addr];
        if (old_data != val) {
            s->enable[addr] = val;
            old_data = old_data ^ val;

            while ((i = find_first_bit((void *)&old_data, 8)) != 8) {
                level = test_bit(i, (unsigned long *)&val);
                extioi_update_irq(s, i + addr * 8, level);
                clear_bit(i, (void *)&old_data);
            }
        }
        break;
    default:
        break;
    }
}

static uint64_t extioi_bounce_coreisr_readw(void *opaque, hwaddr addr,
                                            unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    unsigned long offset = addr & 0xffff;
    uint32_t ret, index;
    int cpu;

    switch (offset) {
    case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END - 1:
        index = (offset - EXTIOI_BOUNCE_START) >> 2;
        ret = s->bounce[index];
        break;
    case EXTIOI_COREISR_START ... EXTIOI_COREISR_END - 1:
        index = ((offset - EXTIOI_COREISR_START) & 0x1f) >> 2;
        cpu = ((offset - EXTIOI_COREISR_START) >> 8) & 0x3;
        ret = s->coreisr[cpu][index];
        break;
    default:
        break;
    }

    trace_loongarch_extioi_bounce_coreisr_readw((uint32_t)addr, ret);
    return ret;
}

static void extioi_bounce_coreisr_writew(void *opaque, hwaddr addr,
                                         uint64_t val, unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    int cpu, index;
    uint32_t offset, old_data, i, j, bits;

    offset = addr & 0xffff;
    trace_loongarch_extioi_bounce_coreisr_writew(size, (uint32_t)addr, val);
    switch (offset) {
    case EXTIOI_BOUNCE_START ... EXTIOI_BOUNCE_END - 1:
        index = (offset - EXTIOI_BOUNCE_START) >> 2;
        s->bounce[index] = val;
        break;
    case EXTIOI_COREISR_START ... EXTIOI_COREISR_END - 1:
        index = ((offset - EXTIOI_COREISR_START) & 0x1f) >> 2;
        cpu = ((offset - EXTIOI_COREISR_START) >> 8) & 0x3;
        old_data = s->coreisr[cpu][index];
        s->coreisr[cpu][index] = old_data & ~val;
        if (old_data != s->coreisr[cpu][index]) {
            bits = size * 8;
            while ((i = find_first_bit((void *)&val, bits)) != bits) {
                j = test_bit(i, (unsigned long *)&old_data);
                if (j) {
                    extioi_update_irq(s, i + index * 32, 0);
                }
                clear_bit(i, (void *)&val);
            }
        }
        break;
    default:
        break;
    }
}

static uint64_t extioi_coremap_read(void *opaque, hwaddr addr, unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    uint8_t ret;

    switch (addr) {
    case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END - 1:
        ret = s->coremap[addr];
        break;
    default:
        break;
    }

    trace_loongarch_extioi_coremap_read((uint8_t)addr, ret);
    return ret;
}

static void extioi_coremap_write(void *opaque, hwaddr addr,
                                 uint64_t value, unsigned size)
{
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(opaque);
    uint8_t val = value & 0xff;
    int cpu;

    trace_loongarch_extioi_coremap_write(size, (uint8_t)addr, val);
    switch (addr) {
    case EXTIOI_COREMAP_START ... EXTIOI_COREMAP_END - 1:
        s->coremap[addr] = val;

        /* Only support 1 node now only handle the core map*/
        if (val) {
            cpu = find_first_bit((void *)&val, 4);
            if (cpu != 4) {
                s->sw_coremap[addr] = cpu;
            }
        }
        break;
    default:
        break;
    }
}

static const MemoryRegionOps extioi_nodetype_ops = {
    .read = extioi_nodetype_readw,
    .write = extioi_nodetype_writew,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 8,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps extioi_ipmap_enable_ops = {
    .read = extioi_ipmap_enable_read,
    .write = extioi_ipmap_enable_write,
    .impl.min_access_size = 1,
    .impl.max_access_size = 1,
    .valid.min_access_size = 1,
    .valid.max_access_size = 8,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps extioi_bounce_coreisr_ops = {
    .read = extioi_bounce_coreisr_readw,
    .write = extioi_bounce_coreisr_writew,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
    .valid.min_access_size = 4,
    .valid.max_access_size = 8,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps extioi_coremap_ops = {
    .read = extioi_coremap_read,
    .write = extioi_coremap_write,
    .impl.min_access_size = 1,
    .impl.max_access_size = 1,
    .valid.min_access_size = 1,
    .valid.max_access_size = 8,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const VMStateDescription vmstate_ext_sw_ipisr = {
    .name = "ext_sw_ipisr",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(irq, ext_sw_ipisr, EXTIOI_IRQS),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_loongarch_extioi = {
    .name = TYPE_LOONGARCH_EXTIOI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(bounce, LoongArchExtIOI, EXTIOI_IRQS_GROUP_COUNT),
        VMSTATE_UINT32_2DARRAY(coreisr, LoongArchExtIOI, MAX_CORES,
                               EXTIOI_IRQS_GROUP_COUNT),
        VMSTATE_UINT32_ARRAY(nodetype, LoongArchExtIOI,
                             EXTIOI_IRQS_NODETYPE_COUNT / 2),
        VMSTATE_UINT8_ARRAY(enable, LoongArchExtIOI, EXTIOI_IRQS / 8),
        VMSTATE_UINT8_ARRAY(ipmap, LoongArchExtIOI, 8),
        VMSTATE_UINT8_ARRAY(coremap, LoongArchExtIOI, EXTIOI_IRQS),
        VMSTATE_UINT8_ARRAY(sw_ipmap, LoongArchExtIOI, EXTIOI_IRQS),
        VMSTATE_UINT8_ARRAY(sw_coremap, LoongArchExtIOI, EXTIOI_IRQS),
        VMSTATE_STRUCT_2DARRAY(sw_ipisr, LoongArchExtIOI, MAX_CORES,
                               LS3A_INTC_IP, 1, vmstate_ext_sw_ipisr,
                               ext_sw_ipisr),
        VMSTATE_END_OF_LIST()
    }
};

static void loongarch_extioi_instance_init(Object *obj)
{
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);
    LoongArchExtIOI *s = LOONGARCH_EXTIOI(obj);
    int i, cpu, pin;

    for (i = 0; i < EXTIOI_IRQS; i++) {
        sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq[i]);
    }

    qdev_init_gpio_in(DEVICE(obj), extioi_setirq, EXTIOI_IRQS);

    for (cpu = 0; cpu < MAX_CORES; cpu++) {
        sysbus_init_mmio(dev, &s->mmio[cpu]);
        for (pin = 0; pin < LS3A_INTC_IP; pin++) {
            qdev_init_gpio_out(DEVICE(obj), &s->parent_irq[cpu][pin], 1);
        }

        memory_region_init(&s->mmio[cpu], OBJECT(s),
                           "loongarch_extioi", EXTIOI_SIZE);

        memory_region_init_io(&s->mmio_nodetype[cpu], OBJECT(s),
                              &extioi_nodetype_ops, s,
                              EXTIOI_LINKNAME(.nodetype),
                              IPMAP_OFFSET - APIC_BASE);
        memory_region_add_subregion(&s->mmio[cpu], 0, &s->mmio_nodetype[cpu]);

        memory_region_init_io(&s->mmio_ipmap_enable[cpu], OBJECT(s),
                              &extioi_ipmap_enable_ops, s,
                              EXTIOI_LINKNAME(.ipmap_enable),
                              BOUNCE_OFFSET - IPMAP_OFFSET);
        memory_region_add_subregion(&s->mmio[cpu], IPMAP_OFFSET - APIC_BASE,
                                    &s->mmio_ipmap_enable[cpu]);

        memory_region_init_io(&s->mmio_bounce_coreisr[cpu], OBJECT(s),
                              &extioi_bounce_coreisr_ops, s,
                              EXTIOI_LINKNAME(.bounce_coreisr),
                              COREMAP_OFFSET - BOUNCE_OFFSET);
        memory_region_add_subregion(&s->mmio[cpu], BOUNCE_OFFSET - APIC_BASE,
                                    &s->mmio_bounce_coreisr[cpu]);

        memory_region_init_io(&s->mmio_coremap[cpu], OBJECT(s),
                              &extioi_coremap_ops, s,
                              EXTIOI_LINKNAME(.coremap),
                              EXTIOI_COREMAP_END);
        memory_region_add_subregion(&s->mmio[cpu], COREMAP_OFFSET - APIC_BASE,
                                    &s->mmio_coremap[cpu]);
    }
}

static void loongarch_extioi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_loongarch_extioi;
}

static const TypeInfo loongarch_extioi_info = {
    .name          = TYPE_LOONGARCH_EXTIOI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_init = loongarch_extioi_instance_init,
    .instance_size = sizeof(struct LoongArchExtIOI),
    .class_init    = loongarch_extioi_class_init,
};

static void loongarch_extioi_register_types(void)
{
    type_register_static(&loongarch_extioi_info);
}

type_init(loongarch_extioi_register_types)
