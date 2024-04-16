## See sim/Makefile.am
##
## Copyright (C) 2004-2024 Free Software Foundation, Inc.
## Contributed by Axis Communications.
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

## Some CGEN kludges are causing build-time warnings.  See cris.cpu for details.
AM_CFLAGS_%C%_mloopv10f.o = $(SIM_CFLAG_WNO_UNUSED_BUT_SET_VARIABLE)
AM_CFLAGS_%C%_mloopv32f.o = $(SIM_CFLAG_WNO_UNUSED_BUT_SET_VARIABLE)
## Some CGEN assignments use variable names that are nested & repeated.
AM_CFLAGS_%C%_mloopv10f.o += $(SIM_CFLAG_WNO_SHADOW_LOCAL)

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
	%D%/crisv10f.o \
	%D%/cpuv10.o \
	%D%/decodev10.o \
	%D%/modelv10.o \
	%D%/mloopv10f.o \
	%D%/crisv32f.o \
	%D%/cpuv32.o \
	%D%/decodev32.o \
	%D%/modelv32.o \
	%D%/mloopv32f.o \
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

%C%_SIM_EXTRA_HW_DEVICES = rv cris cris_900000xx

## rvdummy is just used for testing -- it runs on the same host as `run`.
## It does nothing if --enable-sim-hardware isn't active.
%C%_rvdummy_SOURCES = %D%/rvdummy.c
%C%_rvdummy_LDADD = $(LIBIBERTY_LIB)

check_PROGRAMS += %D%/rvdummy

## List all generated headers to help Automake dependency tracking.
BUILT_SOURCES += \
	%D%/engv10.h \
	%D%/engv32.h
%C%_BUILD_OUTPUTS = \
	%D%/mloopv10f.c \
	%D%/stamp-mloop-v10f \
	%D%/mloopv32f.c \
	%D%/stamp-mloop-v32f

## Generating modules.c requires all sources to scan.
%D%/modules.c: | $(%C%_BUILD_OUTPUTS)

## FIXME: What is mono and what does "Use of `mono' is wip" mean (other
## than the apparent; some "mono" feature is work in progress)?
%D%/mloopv10f.c %D%/engv10.h: %D%/stamp-mloop-v10f ; @true
%D%/stamp-mloop-v10f: %D%/mloop.in $(srccom)/genmloop.sh
	$(AM_V_GEN)$(CGEN_GEN_MLOOP) \
		-mono -no-fast -pbb -switch semcrisv10f-switch.c \
		-cpu crisv10f -outfile-suffix -v10f
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng-v10f.hin %D%/engv10.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop-v10f.cin %D%/mloopv10f.c
	$(AM_V_at)touch $@

## FIXME: What is mono and what does "Use of `mono' is wip" mean (other
## than the apparent; some "mono" feature is work in progress)?
%D%/mloopv32f.c %D%/engv32.h: %D%/stamp-mloop-v32f ; @true
%D%/stamp-mloop-v32f: %D%/mloop.in $(srccom)/genmloop.sh
	$(AM_V_GEN)$(CGEN_GEN_MLOOP) \
		-mono -no-fast -pbb -switch semcrisv32f-switch.c \
		-cpu crisv32f -outfile-suffix -v32f
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng-v32f.hin %D%/engv32.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop-v32f.cin %D%/mloopv32f.c
	$(AM_V_at)touch $@

CLEANFILES += %D%/engv10.h %D%/engv32.h
MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)

## Target that triggers all cgen targets that works when --disable-cgen-maint.
%D%/cgen: %D%/cgen-arch %D%/cgen-cpu-decode-v10f %D%/cgen-cpu-decode-v32f

%D%/cgen-arch:
	$(AM_V_GEN)mach=crisv10,crisv32 FLAGS="with-scache with-profile=fn"; $(CGEN_GEN_ARCH)
$(srcdir)/%D%/arch.h $(srcdir)/%D%/arch.c $(srcdir)/%D%/cpuall.h: @CGEN_MAINT@ %D%/cgen-arch

%D%/cgen-cpu-decode-v10f:
	$(AM_V_GEN)cpu=crisv10f mach=crisv10 SUFFIX=v10 FLAGS="with-scache with-profile=fn" EXTRAFILES="$(CGEN_CPU_SEMSW)"; $(CGEN_GEN_CPU_DECODE)
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change $(srcdir)/%D%/semv10-switch.c $(srcdir)/%D%/semcrisv10f-switch.c
$(srcdir)/%D%/cpuv10.h $(srcdir)/%D%/cpuv10.c $(srcdir)/%D%/semcrisv10f-switch.c $(srcdir)/%D%/modelv10.c $(srcdir)/%D%/decodev10.c $(srcdir)/%D%/decodev10.h: @CGEN_MAINT@ %D%/cgen-cpu-decode-v10f

%D%/cgen-cpu-decode-v32f:
	$(AM_V_GEN)cpu=crisv32f mach=crisv32 SUFFIX=v32 FLAGS="with-scache with-profile=fn" EXTRAFILES="$(CGEN_CPU_SEMSW)"; $(CGEN_GEN_CPU_DECODE)
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change $(srcdir)/%D%/semv32-switch.c $(srcdir)/%D%/semcrisv32f-switch.c
$(srcdir)/%D%/cpuv32.h $(srcdir)/%D%/cpuv32.c $(srcdir)/%D%/semcrisv32f-switch.c $(srcdir)/%D%/modelv32.c $(srcdir)/%D%/decodev32.c $(srcdir)/%D%/decodev32.h: @CGEN_MAINT@ %D%/cgen-cpu-decode-v32f
