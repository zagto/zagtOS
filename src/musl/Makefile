#
# Makefile for musl (requires GNU make)
#
# This is how simple every makefile should be...
# No, I take that back - actually most should be less than half this size.
#
# Use $(SHBUILD_BUILD_DIR)/config.mak to override any of the following variables.
# Do not make changes here.
#

srcdir = .
exec_prefix = /usr/local
bindir = $(exec_prefix)/bin

prefix = /usr/local/musl
includedir = $(prefix)/include
libdir = $(prefix)/lib
syslibdir = /lib

SRC_DIRS = $(addprefix $(srcdir)/,src/* crt ldso $(COMPAT_SRC_DIRS))
BASE_GLOBS = $(addsuffix /*.c,$(SRC_DIRS))
ARCH_GLOBS = $(addsuffix /$(ARCH)/*.[csS],$(SRC_DIRS))
BASE_SRCS = $(sort $(wildcard $(BASE_GLOBS)))
ARCH_SRCS = $(sort $(wildcard $(ARCH_GLOBS)))
BASE_OBJS = $(patsubst $(srcdir)/%,%.o,$(basename $(BASE_SRCS)))
ARCH_OBJS = $(patsubst $(srcdir)/%,%.o,$(basename $(ARCH_SRCS)))
REPLACED_OBJS = $(sort $(subst /$(ARCH)/,/,$(ARCH_OBJS)))
ALL_OBJS = $(addprefix $(SHBUILD_BUILD_DIR)/obj/, $(filter-out $(REPLACED_OBJS), $(sort $(BASE_OBJS) $(ARCH_OBJS))))

LIBC_OBJS = $(filter $(SHBUILD_BUILD_DIR)/obj/src/%,$(ALL_OBJS)) $(filter $(SHBUILD_BUILD_DIR)/obj/compat/%,$(ALL_OBJS))
LDSO_OBJS = $(filter $(SHBUILD_BUILD_DIR)/obj/ldso/%,$(ALL_OBJS:%.o=%.lo))
CRT_OBJS = $(filter $(SHBUILD_BUILD_DIR)/obj/crt/%,$(ALL_OBJS))

AOBJS = $(LIBC_OBJS)
LOBJS = $(LIBC_OBJS:.o=.lo)
GENH = $(SHBUILD_BUILD_DIR)/obj/include/bits/alltypes.h
GENH_INT = $(SHBUILD_BUILD_DIR)/obj/src/internal/version.h
IMPH = $(addprefix $(srcdir)/, src/internal/stdio_impl.h src/internal/pthread_impl.h src/internal/locale_impl.h src/internal/libc.h)

LDFLAGS =
LDFLAGS_AUTO =
LIBCC = -lgcc
CPPFLAGS =
CFLAGS =
CFLAGS_AUTO = -Os -pipe
CFLAGS_C99FSE = -std=c99 -ffreestanding -nostdinc 

CFLAGS_ALL = $(CFLAGS_C99FSE)
CFLAGS_ALL += -D_XOPEN_SOURCE=700 -I$(srcdir)/arch/$(ARCH) -I$(srcdir)/arch/generic -I$(SHBUILD_BUILD_DIR)/obj/src/internal -I$(srcdir)/src/include -I$(srcdir)/src/internal -I$(SHBUILD_BUILD_DIR)/obj/include -I$(srcdir)/include
CFLAGS_ALL += $(CPPFLAGS) $(CFLAGS_AUTO) $(CFLAGS)

LDFLAGS_ALL = $(LDFLAGS_AUTO) $(LDFLAGS)

AR      = $(CROSS_COMPILE)ar
RANLIB  = $(CROSS_COMPILE)ranlib
INSTALL = $(srcdir)/tools/install.sh

ARCH_INCLUDES = $(wildcard $(srcdir)/arch/$(ARCH)/bits/*.h)
GENERIC_INCLUDES = $(wildcard $(srcdir)/arch/generic/bits/*.h)
INCLUDES = $(wildcard $(srcdir)/include/*.h $(srcdir)/include/*/*.h)
ALL_INCLUDES = $(sort $(INCLUDES:$(srcdir)/%=%) $(GENH:$(SHBUILD_BUILD_DIR)/obj/%=%) $(ARCH_INCLUDES:$(srcdir)/arch/$(ARCH)/%=include/%) $(GENERIC_INCLUDES:$(srcdir)/arch/generic/%=include/%))

EMPTY_LIB_NAMES = m rt pthread crypt util xnet resolv dl
EMPTY_LIBS = $(EMPTY_LIB_NAMES:%=$(SHBUILD_BUILD_DIR)/lib/lib%.a)
CRT_LIBS = $(addprefix $(SHBUILD_BUILD_DIR)/lib/,$(notdir $(CRT_OBJS)))
STATIC_LIBS = $(SHBUILD_BUILD_DIR)/lib/libc.a
SHARED_LIBS = $(SHBUILD_BUILD_DIR)/lib/libc.so
TOOL_LIBS = $(SHBUILD_BUILD_DIR)/lib/musl-gcc.specs
ALL_LIBS = $(CRT_LIBS) $(STATIC_LIBS) $(SHARED_LIBS) $(EMPTY_LIBS) $(TOOL_LIBS)
ALL_TOOLS = $(SHBUILD_BUILD_DIR)/obj/musl-gcc

WRAPCC_GCC = gcc
WRAPCC_CLANG = clang

LDSO_PATHNAME = $(syslibdir)/ld-musl-$(ARCH)$(SUBARCH).so.1

-include $(SHBUILD_BUILD_DIR)/config.mak
-include $(srcdir)/arch/$(ARCH)/arch.mak

ifeq ($(ARCH),)

all:
	@echo "Please set ARCH in $(SHBUILD_BUILD_DIR)/config.mak before running make."
	@exit 1

else

all: $(ALL_LIBS) $(ALL_TOOLS)

OBJ_DIRS = $(sort $(patsubst %/,%,$(dir $(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(GENH) $(GENH_INT))) $(SHBUILD_BUILD_DIR)/obj/include)

$(ALL_LIBS) $(ALL_TOOLS) $(ALL_OBJS) $(ALL_OBJS:%.o=%.lo) $(GENH) $(GENH_INT): | $(OBJ_DIRS)

$(OBJ_DIRS):
	mkdir -p $@

$(SHBUILD_BUILD_DIR)/obj/include/bits/alltypes.h: $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in $(srcdir)/tools/mkalltypes.sed
	sed -f $(srcdir)/tools/mkalltypes.sed $(srcdir)/arch/$(ARCH)/bits/alltypes.h.in $(srcdir)/include/alltypes.h.in > $@

$(SHBUILD_BUILD_DIR)/obj/include/bits/syscall.h: $(srcdir)/arch/$(ARCH)/bits/syscall.h.in
	cp $< $@
	sed -n -e s/__NR_/SYS_/p < $< >> $@

$(SHBUILD_BUILD_DIR)/obj/src/internal/version.h: $(wildcard $(srcdir)/VERSION $(srcdir)/.git)
	printf '#define VERSION "%s"\n' "$$(cd $(srcdir); sh tools/version.sh)" > $@

$(SHBUILD_BUILD_DIR)/obj/src/internal/version.o $(SHBUILD_BUILD_DIR)/obj/src/internal/version.lo: $(SHBUILD_BUILD_DIR)/obj/src/internal/version.h

OPTIMIZE_SRCS = $(wildcard $(OPTIMIZE_GLOBS:%=$(srcdir)/src/%))
$(OPTIMIZE_SRCS:$(srcdir)/%.c=$(SHBUILD_BUILD_DIR)/obj/%.o) $(OPTIMIZE_SRCS:$(srcdir)/%.c=$(SHBUILD_BUILD_DIR)/obj/%.lo): CFLAGS += -O3

MEMOPS_OBJS = $(filter %/memcpy.o %/memmove.o %/memcmp.o %/memset.o, $(LIBC_OBJS))
$(MEMOPS_OBJS) $(MEMOPS_OBJS:%.o=%.lo): CFLAGS_ALL += $(CFLAGS_MEMOPS)

NOSSP_OBJS = $(CRT_OBJS) $(LDSO_OBJS) $(filter \
	%/__libc_start_main.o %/__init_tls.o %/__stack_chk_fail.o \
	%/__set_thread_area.o %/memset.o %/memcpy.o \
	, $(LIBC_OBJS))
$(NOSSP_OBJS) $(NOSSP_OBJS:%.o=%.lo): CFLAGS_ALL += $(CFLAGS_NOSSP)

$(CRT_OBJS): CFLAGS_ALL += -DCRT

$(LOBJS) $(LDSO_OBJS): CFLAGS_ALL += -fPIC

CC_CMD = $(CC) $(CFLAGS_ALL) -c -o $@ $<

# Choose invocation of assembler to be used
ifeq ($(ADD_CFI),yes)
	AS_CMD = LC_ALL=C awk -f $(srcdir)/tools/add-cfi.common.awk -f $(srcdir)/tools/add-cfi.$(ARCH).awk $< | $(CC) $(CFLAGS_ALL) -x assembler -c -o $@ -
else
	AS_CMD = $(CC_CMD)
endif

$(SHBUILD_BUILD_DIR)/obj/%.o: $(srcdir)/%.s
	$(AS_CMD)

$(SHBUILD_BUILD_DIR)/obj/%.o: $(srcdir)/%.S
	$(CC_CMD)

$(SHBUILD_BUILD_DIR)/obj/%.o: $(srcdir)/%.c $(GENH) $(IMPH)
	$(CC_CMD)

$(SHBUILD_BUILD_DIR)/obj/%.lo: $(srcdir)/%.s
	$(AS_CMD)

$(SHBUILD_BUILD_DIR)/obj/%.lo: $(srcdir)/%.S
	$(CC_CMD)

$(SHBUILD_BUILD_DIR)/obj/%.lo: $(srcdir)/%.c $(GENH) $(IMPH)
	$(CC_CMD)

$(SHBUILD_BUILD_DIR)/lib/libc.so: $(LOBJS) $(LDSO_OBJS)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS_ALL) -nostdlib -shared \
	-Wl,-e,_dlstart -o $@ $(LOBJS) $(LDSO_OBJS) $(LIBCC)

$(SHBUILD_BUILD_DIR)/lib/libc.a: $(AOBJS)
	rm -f $@
	$(AR) rc $@ $(AOBJS)
	$(RANLIB) $@

$(EMPTY_LIBS):
	rm -f $@
	$(AR) rc $@

$(SHBUILD_BUILD_DIR)/lib/%.o: $(SHBUILD_BUILD_DIR)/obj/crt/$(ARCH)/%.o
	cp $< $@

$(SHBUILD_BUILD_DIR)/lib/%.o: $(SHBUILD_BUILD_DIR)/obj/crt/%.o
	cp $< $@

$(SHBUILD_BUILD_DIR)/lib/musl-gcc.specs: $(srcdir)/tools/musl-gcc.specs.sh $(SHBUILD_BUILD_DIR)/config.mak
	sh $< "$(includedir)" "$(libdir)" "$(LDSO_PATHNAME)" > $@

$(SHBUILD_BUILD_DIR)/obj/musl-gcc: $(SHBUILD_BUILD_DIR)/config.mak
	printf '#!/bin/sh\nexec "$${REALGCC:-$(WRAPCC_GCC)}" "$$@" -specs "%s/musl-gcc.specs"\n' "$(libdir)" > $@
	chmod +x $@

$(SHBUILD_BUILD_DIR)/obj/%-clang: $(srcdir)/tools/%-clang.in $(SHBUILD_BUILD_DIR)/config.mak
	sed -e 's!@CC@!$(WRAPCC_CLANG)!g' -e 's!@PREFIX@!$(prefix)!g' -e 's!@INCDIR@!$(includedir)!g' -e 's!@LIBDIR@!$(libdir)!g' -e 's!@LDSO@!$(LDSO_PATHNAME)!g' $< > $@
	chmod +x $@

$(DESTDIR)$(bindir)/%: $(SHBUILD_BUILD_DIR)/obj/%
	$(INSTALL) -D $< $@

$(DESTDIR)$(libdir)/%.so: $(SHBUILD_BUILD_DIR)/lib/%.so
	$(INSTALL) -D -m 755 $< $@

$(DESTDIR)$(libdir)/%: $(SHBUILD_BUILD_DIR)/lib/%
	$(INSTALL) -D -m 644 $< $@

$(DESTDIR)$(includedir)/bits/%: $(srcdir)/arch/$(ARCH)/bits/%
	$(INSTALL) -D -m 644 $< $@

$(DESTDIR)$(includedir)/bits/%: $(srcdir)/arch/generic/bits/%
	$(INSTALL) -D -m 644 $< $@

$(DESTDIR)$(includedir)/bits/%: $(SHBUILD_BUILD_DIR)/obj/include/bits/%
	$(INSTALL) -D -m 644 $< $@

$(DESTDIR)$(includedir)/%: $(srcdir)/include/%
	$(INSTALL) -D -m 644 $< $@

$(DESTDIR)$(LDSO_PATHNAME): $(DESTDIR)$(libdir)/libc.so
	$(INSTALL) -D -l $(libdir)/libc.so $@ || true

install-libs: $(ALL_LIBS:$(SHBUILD_BUILD_DIR)/lib/%=$(DESTDIR)$(libdir)/%) $(if $(SHARED_LIBS),$(DESTDIR)$(LDSO_PATHNAME),)

install-headers: $(ALL_INCLUDES:include/%=$(DESTDIR)$(includedir)/%)

install-tools: $(ALL_TOOLS:$(SHBUILD_BUILD_DIR)/obj/%=$(DESTDIR)$(bindir)/%)

install: install-libs install-headers install-tools

musl-git-%.tar.gz: .git
	 git --git-dir=$(srcdir)/.git archive --format=tar.gz --prefix=$(patsubst %.tar.gz,%,$@)/ -o $@ $(patsubst musl-git-%.tar.gz,%,$@)

musl-%.tar.gz: .git
	 git --git-dir=$(srcdir)/.git archive --format=tar.gz --prefix=$(patsubst %.tar.gz,%,$@)/ -o $@ v$(patsubst musl-%.tar.gz,%,$@)

endif

clean:
	rm -rf $(SHBUILD_BUILD_DIR)/obj $(SHBUILD_BUILD_DIR)/lib

distclean: clean
	rm -f $(SHBUILD_BUILD_DIR)/config.mak

.PHONY: all clean install install-libs install-headers install-tools
