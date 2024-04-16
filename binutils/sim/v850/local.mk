## See sim/Makefile.am
##
## Copyright (C) 1996-2024 Free Software Foundation, Inc.
## Written by Cygnus Support.
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
	%D%/simops.o \
	%D%/interp.o \
	%D%/itable.o \
	%D%/semantics.o \
	%D%/idecode.o \
	%D%/icache.o \
	%D%/engine.o \
	%D%/irun.o \
	%D%/support.o \
	%D%/sim-resume.o
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

## List all generated headers to help Automake dependency tracking.
BUILT_SOURCES += \
	%D%/icache.h \
	%D%/idecode.h \
	%D%/semantics.h \
	%D%/model.h \
	%D%/support.h \
	%D%/itable.h \
	%D%/engine.h
%C%_BUILT_SRC_FROM_IGEN = \
	%D%/icache.h \
	%D%/icache.c \
	%D%/idecode.h \
	%D%/idecode.c \
	%D%/semantics.h \
	%D%/semantics.c \
	%D%/model.h \
	%D%/model.c \
	%D%/support.h \
	%D%/support.c \
	%D%/itable.h \
	%D%/itable.c \
	%D%/engine.h \
	%D%/engine.c \
	%D%/irun.c
%C%_BUILD_OUTPUTS = \
	$(%C%_BUILT_SRC_FROM_IGEN) \
	%D%/stamp-igen

## Generating modules.c requires all sources to scan.
%D%/modules.c: | $(%C%_BUILD_OUTPUTS)

$(%C%_BUILT_SRC_FROM_IGEN): %D%/stamp-igen

%C%_IGEN_TRACE = # -G omit-line-numbers # -G trace-rule-selection -G trace-rule-rejection -G trace-entries
%C%_IGEN_INSN = $(srcdir)/%D%/v850.igen
%C%_IGEN_DC = $(srcdir)/%D%/v850.dc
%D%/stamp-igen: $(%C%_IGEN_INSN) $(%C%_IGEN_DC) $(IGEN)
	$(AM_V_GEN)$(IGEN_RUN) \
		$(%C%_IGEN_TRACE) \
		-G gen-direct-access \
		-G gen-zero-r0 \
		-i $(%C%_IGEN_INSN) \
		-o $(%C%_IGEN_DC) \
		-x \
		-n icache.h    -hc %D%/icache.h \
		-n icache.c    -c  %D%/icache.c \
		-n semantics.h -hs %D%/semantics.h \
		-n semantics.c -s  %D%/semantics.c \
		-n idecode.h   -hd %D%/idecode.h \
		-n idecode.c   -d  %D%/idecode.c \
		-n model.h     -hm %D%/model.h \
		-n model.c     -m  %D%/model.c \
		-n support.h   -hf %D%/support.h \
		-n support.c   -f  %D%/support.c \
		-n itable.h    -ht %D%/itable.h \
		-n itable.c    -t  %D%/itable.c \
		-n engine.h    -he %D%/engine.h \
		-n engine.c    -e  %D%/engine.c \
		-n irun.c      -r  %D%/irun.c
	$(AM_V_at)touch $@

MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
