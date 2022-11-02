# -*- Mode: makefile -*-
#
# This Makefile example is fairly independent from the main makefile
# so users can take and adapt it for their build. We only really
# include config-host.mak so we don't have to repeat probing for
# cflags that the main configure has already done for us.
#

BUILD_DIR := $(CURDIR)/../../../..

include $(BUILD_DIR)/config-host.mak

VPATH += $(SRC_PATH)/target/loongarch/pin/pintools
NAMES :=
NAMES += template
NAMES += ins_count
NAMES += bbl_count
NAMES += inline_add
NAMES += strace
NAMES += routine
NAMES += shared_object
NAMES += bcc
NAMES += reg_frequency
NAMES += time_in_code_cache

SONAMES := $(addsuffix .so,$(addprefix lib,$(NAMES)))

# The main QEMU uses Glib extensively so it's perfectly fine to use it
# in plugins (which many example do).
CFLAGS = $(GLIB_CFLAGS)
CFLAGS += -fPIC -Wall $(filter -W%, $(QEMU_CFLAGS))
CFLAGS += $(if $(findstring no-psabi,$(QEMU_CFLAGS)),-Wpsabi)
CFLAGS += -I$(SRC_PATH)/include/qemu

# 下面这些是我自己加的
CFLAGS += -I$(SRC_PATH)/target/loongarch -I$(SRC_PATH)/include -I$(SRC_PATH) -I$(BUILD_DIR)
CFLAGS += -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DNEED_CPU_H
CFLAGS += -DCONFIG_TARGET=\"loongarch64-linux-user-config-target.h\" -DCONFIG_DEVICES=\"loongarch64-linux-user-config-devices.h\"
CFLAGS += -DCONFIG_LMJ -DCONFIG_LMJ_DEBUG
# 没有加上 CONFIG_USER_ONLY 也编过了。。
# CFLAGS += -DCONFIG_USER_ONLY -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DCONFIG_LMJ -DCONFIG_LMJ_DEBUG -DNEED_CPU_H -DCONFIG_TARGET=\"loongarch64-linux-user-config-target.h\" -DCONFIG_DEVICES=\"loongarch64-linux-user-config-devices.h\"

## compile_commands.json
# {
#   "directory": "/home/lmj/code/qemu/build",
#   "command": "cc -m64 -mcx16 -Ilibqemu-loongarch64-linux-user.fa.p -I. -I.. -Itarget/loongarch -I../target/loongarch -I../common-user/host/x86_64 -I../linux-user/include/host/x86_64 -I../linux-user/include -Ilinux-user -I../linux-user -I../linux-user/loongarch64 -I../capstone/include/capstone -Iqapi -Itrace -Iui/shader -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -fdiagnostics-color=auto -Wall -Winvalid-pch -Werror -std=gnu11 -g -isystem /home/lmj/code/qemu/linux-headers -isystem linux-headers -iquote . -iquote /home/lmj/code/qemu -iquote /home/lmj/code/qemu/include -iquote /home/lmj/code/qemu/disas/libvixl -iquote /home/lmj/code/qemu/tcg/i386 -pthread -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wstrict-prototypes -Wredundant-decls -Wundef -Wwrite-strings -Wmissing-prototypes -fno-strict-aliasing -fno-common -fwrapv -Wold-style-declaration -Wold-style-definition -Wtype-limits -Wformat-security -Wformat-y2k -Winit-self -Wignored-qualifiers -Wempty-body -Wnested-externs -Wendif-labels -Wexpansion-to-defined -Wimplicit-fallthrough=2 -Wno-missing-include-dirs -Wno-shift-negative-value -Wno-psabi -fstack-protector-strong -DCONFIG_LMJ -DCONFIG_LMJ_DEBUG -fPIE -isystem../linux-headers -isystemlinux-headers -DNEED_CPU_H '-DCONFIG_TARGET=\"loongarch64-linux-user-config-target.h\"' '-DCONFIG_DEVICES=\"loongarch64-linux-user-config-devices.h\"' -MD -MQ libqemu-loongarch64-linux-user.fa.p/target_loongarch_pin_pintools_template.c.o -MF libqemu-loongarch64-linux-user.fa.p/target_loongarch_pin_pintools_template.c.o.d -o libqemu-loongarch64-linux-user.fa.p/target_loongarch_pin_pintools_template.c.o -c ../target/loongarch/pin/pintools/template.c",
#   "file": "../target/loongarch/pin/pintools/template.c",
#   "output": "libqemu-loongarch64-linux-user.fa.p/target_loongarch_pin_pintools_template.c.o"
# },


all: $(SONAMES)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

lib%.so: %.o
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o *.so *.d
	rm -Rf .libs

.PHONY: all clean

