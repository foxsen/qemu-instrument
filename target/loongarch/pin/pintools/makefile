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
NAMES += hotblock
NAMES += inline_add
NAMES += strace
NAMES += routine
NAMES += bcc
NAMES += reg_frequency
NAMES += time_in_code_cache
NAMES += branch_count
NAMES += isampling
NAMES += branch_offset

SONAMES := $(addsuffix .so,$(addprefix lib,$(NAMES)))

# The main QEMU uses Glib extensively so it's perfectly fine to use it
# in plugins (which many example do).
CFLAGS = $(GLIB_CFLAGS)
CFLAGS += -fPIC -Wall $(filter -W%, $(QEMU_CFLAGS))
CFLAGS += $(if $(findstring no-psabi,$(QEMU_CFLAGS)),-Wpsabi)
CFLAGS += -I$(SRC_PATH)/include

# 照着CFLAGS抄的
CXXFLAGS = $(GLIB_CFLAGS)
CXXFLAGS += -fPIC -Wall $(filter -W%, $(QEMU_CXXFLAGS))
CXXFLAGS += $(if $(findstring no-psabi,$(QEMU_CXXFLAGS)),-Wpsabi)
CXXFLAGS += -I$(SRC_PATH)/include
CXXFLAGS += -std=gnu++11

all: $(SONAMES)

# FIXME write a rule for general .cpp file
hotblock.o: hotblock.cpp
	c++ $(CXXFLAGS) -c -o $@ $<

isampling.o: isampling.cpp
	c++ $(CXXFLAGS) -c -o $@ $<

branch_offset.o: branch_offset.cpp
	c++ $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

lib%.so: %.o
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o *.so *.d
	rm -Rf .libs

.PHONY: all clean

