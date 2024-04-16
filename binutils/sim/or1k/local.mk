## See sim/Makefile.am
##
## Copyright (C) 2017-2024 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

AM_CPPFLAGS_%C% = -DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31

nodist_%C%_libsim_a_SOURCES = \
	%D%/modules.c
%C%_libsim_a_SOURCES = \
	$(common_libcommon_a_SOURCES)
%C%_libsim_a_LIBADD = \
	$(patsubst %,%D%/%,$(SIM_NEW_COMMON_OBJS)) \
	$(patsubst %,%D%/dv-%.o,$(SIM_HW_DEVICES)) \
	\
	%D%/cgen-accfp.o \
	%D%/cgen-fpu.o \
	%D%/cgen-run.o \
	%D%/cgen-scache.o \
	%D%/cgen-trace.o \
	%D%/cgen-utils.o \
	\
	%D%/arch.o \
	%D%/cpu.o \
	%D%/decode.o \
	%D%/mloop.o \
	%D%/model.o \
	%D%/sem.o \
	\
	%D%/or1k.o \
	%D%/sim-if.o \
	%D%/traps.o
$(%C%_libsim_a_OBJECTS) $(%C%_libsim_a_LIBADD): %D%/hw-config.h

noinst_LIBRARIES += %D%/libsim.a

## Override wildcards that trigger common/modules.c to be (incorrectly) used.
%D%/modules.o: %D%/modules.c

%D%/%.o: common/%.c ; $(SIM_COMPILE)
-@am__include@ %D%/$(DEPDIR)/*.Po

%C%_run_SOURCES =
%C%_run_LDADD = \
	%D%/nrun.o \
	%D%/libsim.a \
	$(SIM_COMMON_LIBS)

noinst_PROGRAMS += %D%/run

%C%docdir = $(docdir)/%C%
%C%doc_DATA = %D%/README

## List all generated headers to help Automake dependency tracking.
BUILT_SOURCES += %D%/eng.h
%C%_BUILD_OUTPUTS = \
	%D%/mloop.c \
	%D%/stamp-mloop

## Generating modules.c requires all sources to scan.
%D%/modules.c: | $(%C%_BUILD_OUTPUTS)

## FIXME: Use of `mono' is wip.
%D%/mloop.c %D%/eng.h: %D%/stamp-mloop ; @true
%D%/stamp-mloop: %D%/mloop.in $(srccom)/genmloop.sh
	$(AM_V_GEN)$(CGEN_GEN_MLOOP) \
		-mono -fast -pbb -switch sem-switch.c \
		-cpu or1k32bf
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng.hin %D%/eng.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop.cin %D%/mloop.c
	$(AM_V_at)touch $@

CLEANFILES += %D%/eng.h
MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)

## Target that triggers all cgen targets that works when --disable-cgen-maint.
%D%/cgen: %D%/cgen-arch %D%/cgen-cpu-decode

%D%/cgen-arch:
	$(AM_V_GEN)mach=or32,or32nd FLAGS="with-scache"; $(CGEN_GEN_ARCH)
$(srcdir)/%D%/arch.h $(srcdir)/%D%/arch.c $(srcdir)/%D%/cpuall.h: @CGEN_MAINT@ %D%/cgen-arch

%D%/cgen-cpu-decode:
	$(AM_V_GEN)cpu=or1k32bf mach=or32,or32nd FLAGS="with-scache" EXTRAFILES="$(CGEN_CPU_SEM) $(CGEN_CPU_SEMSW)"; $(CGEN_GEN_CPU_DECODE)
$(srcdir)/%D%/cpu.h $(srcdir)/%D%/cpu.c $(srcdir)/%D%/model.c $(srcdir)/%D%/sem.c $(srcdir)/%D%/sem-switch.c $(srcdir)/%D%/decode.c $(srcdir)/%D%/decode.h: @CGEN_MAINT@ %D%/cgen-cpu-decode
