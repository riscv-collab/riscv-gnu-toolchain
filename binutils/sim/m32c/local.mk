## See sim/Makefile.am
##
## Copyright (C) 2005-2024 Free Software Foundation, Inc.
## Contributed by Red Hat, Inc.
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

AM_CPPFLAGS_%C% = -DTIMER_A

nodist_%C%_libsim_a_SOURCES = \
	%D%/modules.c
%C%_libsim_a_SOURCES = \
	$(common_libcommon_a_SOURCES)
%C%_libsim_a_LIBADD = \
	%D%/gdb-if.o \
	%D%/int.o \
	%D%/load.o \
	%D%/m32c.o \
	%D%/mem.o \
	%D%/misc.o \
	%D%/r8c.o \
	%D%/reg.o \
	%D%/srcdest.o \
	%D%/syscalls.o \
	%D%/trace.o
$(%C%_libsim_a_OBJECTS) $(%C%_libsim_a_LIBADD): %D%/hw-config.h

noinst_LIBRARIES += %D%/libsim.a

## Override wildcards that trigger common/modules.c to be (incorrectly) used.
%D%/modules.o: %D%/modules.c

%D%/%.o: common/%.c ; $(SIM_COMPILE)
-@am__include@ %D%/$(DEPDIR)/*.Po

%C%_run_SOURCES =
%C%_run_LDADD = \
	%D%/main.o \
	%D%/libsim.a \
	$(SIM_COMMON_LIBS)

noinst_PROGRAMS += %D%/run

%C%_BUILD_OUTPUTS = \
	%D%/opc2c$(EXEEXT) \
	%D%/m32c.c \
	%D%/r8c.c

## Generating modules.c requires all sources to scan.
%D%/modules.c: | $(%C%_BUILD_OUTPUTS)

%C%_opc2c_SOURCES = %D%/opc2c.c

# These rules are copied from automake, but tweaked to use FOR_BUILD variables.
%D%/opc2c$(EXEEXT): $(%C%_opc2c_OBJECTS) $(%C%_opc2c_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_opc2c_OBJECTS) $(%C%_opc2c_LDADD)

# opc2c is a build-time only tool.  Override the default rules for it.
%D%/opc2c.o: %D%/opc2c.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@

# opc2c leaks memory, and therefore makes AddressSanitizer unhappy.  Disable
# leak detection while running it.
%C%_OPC2C_RUN = ASAN_OPTIONS=detect_leaks=0 %D%/opc2c$(EXEEXT)

%D%/m32c.c: %D%/m32c.opc %D%/opc2c$(EXEEXT)
	$(AM_V_GEN)$(%C%_OPC2C_RUN) -l $@.log $< > $@.tmp
	$(AM_V_at)mv $@.tmp $@

%D%/r8c.c: %D%/r8c.opc %D%/opc2c$(EXEEXT)
	$(AM_V_GEN)$(%C%_OPC2C_RUN) -l $@.log $< > $@.tmp
	$(AM_V_at)mv $@.tmp $@

EXTRA_PROGRAMS += %D%/opc2c
MOSTLYCLEANFILES += \
	$(%C%_BUILD_OUTPUTS) \
	%D%/m32c.c.log \
	%D%/r8c.c.log
