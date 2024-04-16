## See sim/Makefile.am
##
## Copyright (C) 1996-2024 Free Software Foundation, Inc.
## Contributed by Cygnus Support.
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

nodist_%C%_libsim_a_SOURCES = \
	%D%/modules.c
%C%_libsim_a_SOURCES = \
	$(common_libcommon_a_SOURCES)
%C%_libsim_a_LIBADD = \
	$(patsubst %,%D%/%,$(SIM_NEW_COMMON_OBJS)) \
	$(patsubst %,%D%/dv-%.o,$(SIM_HW_DEVICES)) \
	$(patsubst %,%D%/dv-%.o,$(%C%_SIM_EXTRA_HW_DEVICES)) \
	\
	%D%/cgen-run.o \
	%D%/cgen-scache.o \
	%D%/cgen-trace.o \
	%D%/cgen-utils.o \
	\
	%D%/arch.o \
	\
	%D%/m32r.o \
	%D%/cpu.o \
	%D%/decode.o \
	%D%/sem.o \
	%D%/model.o \
	%D%/mloop.o \
	\
	%D%/m32rx.o \
	%D%/cpux.o \
	%D%/decodex.o \
	%D%/modelx.o \
	%D%/mloopx.o \
	\
	%D%/m32r2.o \
	%D%/cpu2.o \
	%D%/decode2.o \
	%D%/model2.o \
	%D%/mloop2.o \
	\
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

%C%_SIM_EXTRA_HW_DEVICES = m32r_cache m32r_uart

## List all generated headers to help Automake dependency tracking.
BUILT_SOURCES += \
	%D%/eng.h \
	%D%/engx.h \
	%D%/eng2.h
%C%_BUILD_OUTPUTS = \
	%D%/mloop.c \
	%D%/stamp-mloop \
	%D%/mloopx.c \
	%D%/stamp-mloop-x \
	%D%/mloop2.c \
	%D%/stamp-mloop-2

## Generating modules.c requires all sources to scan.
%D%/modules.c: | $(%C%_BUILD_OUTPUTS)

## FIXME: Use of `mono' is wip.
%D%/mloop.c %D%/eng.h: %D%/stamp-mloop ; @true
%D%/stamp-mloop: %D%/mloop.in $(srccom)/genmloop.sh
	$(AM_V_GEN)$(CGEN_GEN_MLOOP) \
		-mono -fast -pbb -switch sem-switch.c \
		-cpu m32rbf
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng.hin %D%/eng.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop.cin %D%/mloop.c
	$(AM_V_at)touch $@

## FIXME: Use of `mono' is wip.
%D%/mloopx.c %D%/engx.h: %D%/stamp-mloop-x ; @true
%D%/stamp-mloop-x: %D%/mloopx.in $(srccom)/genmloop.sh
	$(AM_V_GEN)$(CGEN_GEN_MLOOP) \
		-mono -no-fast -pbb -parallel-write -switch semx-switch.c \
		-cpu m32rxf -outfile-suffix x
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/engx.hin %D%/engx.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloopx.cin %D%/mloopx.c
	$(AM_V_at)touch $@

## FIXME: Use of `mono' is wip.
%D%/mloop2.c %D%/eng2.h: %D%/stamp-mloop-2 ; @true
%D%/stamp-mloop-2: %D%/mloop2.in $(srccom)/genmloop.sh
	$(AM_V_GEN)$(CGEN_GEN_MLOOP) \
		-mono -no-fast -pbb -parallel-write -switch sem2-switch.c \
		-cpu m32r2f -outfile-suffix 2
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng2.hin %D%/eng2.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop2.cin %D%/mloop2.c
	$(AM_V_at)touch $@

CLEANFILES += %D%/eng.h %D%/engx.h %D%/eng2.h
MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)

## Target that triggers all cgen targets that works when --disable-cgen-maint.
%D%/cgen: %D%/cgen-arch %D%/cgen-cpu-decode %D%/cgen-cpu-decode-x %D%/cgen-cpu-decode-2

%D%/cgen-arch:
	$(AM_V_GEN)mach=all FLAGS="with-scache with-profile=fn"; $(CGEN_GEN_ARCH)
$(srcdir)/%D%/arch.h $(srcdir)/%D%/arch.c $(srcdir)/%D%/cpuall.h: @CGEN_MAINT@ %D%/cgen-arch

%D%/cgen-cpu-decode:
	$(AM_V_GEN)cpu=m32rbf mach=m32r FLAGS="with-scache with-profile=fn" EXTRAFILES="$(CGEN_CPU_SEM) $(CGEN_CPU_SEMSW)"; $(CGEN_GEN_CPU_DECODE)
$(srcdir)/%D%/cpu.h $(srcdir)/%D%/sem.c $(srcdir)/%D%/sem-switch.c $(srcdir)/%D%/model.c $(srcdir)/%D%/decode.c $(srcdir)/%D%/decode.h: @CGEN_MAINT@ %D%/cgen-cpu-decode

%D%/cgen-cpu-decode-x:
	$(AM_V_GEN)cpu=m32rxf mach=m32rx SUFFIX=x FLAGS="with-scache with-profile=fn" EXTRAFILES="$(CGEN_CPU_SEMSW)"; $(CGEN_GEN_CPU_DECODE)
$(srcdir)/%D%/cpux.h $(srcdir)/%D%/semx-switch.c $(srcdir)/%D%/modelx.c $(srcdir)/%D%/decodex.c $(srcdir)/%D%/decodex.h: @CGEN_MAINT@ %D%/cgen-cpu-decode-x

%D%/cgen-cpu-decode-2:
	$(AM_V_GEN)cpu=m32r2f mach=m32r2 SUFFIX=2 FLAGS="with-scache with-profile=fn" EXTRAFILES="$(CGEN_CPU_SEMSW)"; $(CGEN_GEN_CPU_DECODE)
$(srcdir)/%D%/cpu2.h $(srcdir)/%D%/sem2-switch.c $(srcdir)/%D%/model2.c $(srcdir)/%D%/decode2.c $(srcdir)/%D%/decode2.h: @CGEN_MAINT@ %D%/cgen-cpu-decode-2
