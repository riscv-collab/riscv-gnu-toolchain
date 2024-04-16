## See sim/Makefile.am
##
## Copyright (C) 2008-2024 Free Software Foundation, Inc.
## Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>
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
	%D%/interp.o \
	%D%/sim-resume.o \
	%D%/simops.o \
	%D%/table.o
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
BUILT_SOURCES += %D%/simops.h
%C%_BUILD_OUTPUTS = \
	%D%/gencode$(EXEEXT) \
	%D%/table.c

## Generating modules.c requires all sources to scan.
%D%/modules.c: | $(%C%_BUILD_OUTPUTS)

%C%_gencode_SOURCES = %D%/gencode.c
%C%_gencode_LDADD = %D%/cr16-opc.o

# These rules are copied from automake, but tweaked to use FOR_BUILD variables.
%D%/gencode$(EXEEXT): $(%C%_gencode_OBJECTS) $(%C%_gencode_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_gencode_OBJECTS) $(%C%_gencode_LDADD)

# gencode is a build-time only tool.  Override the default rules for it.
%D%/gencode.o: %D%/gencode.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@
%D%/cr16-opc.o: ../opcodes/cr16-opc.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@

%D%/simops.h: %D%/gencode$(EXEEXT)
	$(AM_V_GEN)$< -h >$@

%D%/table.c: %D%/gencode$(EXEEXT)
	$(AM_V_GEN)$< >$@

EXTRA_PROGRAMS += %D%/gencode
CLEANFILES += %D%/simops.h
MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
