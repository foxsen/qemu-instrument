loongson3 virt generic platform (``virt``)
====================================================

The ``virt`` machine use gpex host bridge, and there are some
emulated devices on virt board, such as loongson7a RTC device,
IOAPIC device, ACPI device and so on.

Supported devices
-----------------

The ``virt`` machine supports:
- Gpex host bridge
- Ls7a RTC device
- Ls7a IOAPIC device
- Ls7a ACPI device
- Fw_cfg device
- PCI/PCIe devices
- Memory device
- CPU device. Type: Loongson-3A5000.

CPU and machine Type
--------------------

The ``qemu-system-loongarch64`` provides emulation for virt
machine. You can specify the machine type ``virt`` and
cpu type ``Loongson-3A5000``.

Boot options
------------

Now the ``virt`` machine can start using -bios parameter:

.. code-block:: bash

  $ qemu-system-loongarch64 -M virt -cpu Loongson-3A5000 -smp 2 -m 1G \
      -display none -serial stdio \
      -bios loongarch_bios.bin
