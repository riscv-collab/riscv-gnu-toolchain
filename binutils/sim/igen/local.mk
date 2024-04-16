## The IGEN simulator generator for GDB, the GNU Debugger.
##
## Copyright 2002-2024 Free Software Foundation, Inc.
##
## Contributed by Andrew Cagney.
##
## This file is part of GDB.
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

# igen leaks memory, and therefore makes AddressSanitizer unhappy.  Disable
# leak detection while running it.
IGEN = %D%/igen$(EXEEXT)
IGEN_RUN = ASAN_OPTIONS=detect_leaks=0 $(IGEN) $(IGEN_FLAGS_SMP)

# Alias for developers.
igen: $(IGEN)

EXTRA_LIBRARIES += %D%/libigen.a
%C%_libigen_a_SOURCES = \
	%D%/table.c \
	%D%/lf.c \
	%D%/misc.c \
	%D%/filter_host.c \
	%D%/ld-decode.c \
	%D%/ld-cache.c \
	%D%/filter.c \
	%D%/ld-insn.c \
	%D%/gen-model.c \
	%D%/gen-itable.c \
	%D%/gen-icache.c \
	%D%/gen-semantics.c \
	%D%/gen-idecode.c \
	%D%/gen-support.c \
	%D%/gen-engine.c \
	%D%/gen.c

%C%_igen_SOURCES = %D%/igen.c
%C%_igen_LDADD = %D%/libigen.a

# These rules are copied from automake, but tweaked to use FOR_BUILD variables.
%D%/libigen.a: $(igen_libigen_a_OBJECTS) $(igen_libigen_a_DEPENDENCIES) $(EXTRA_igen_libigen_a_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_at)-rm -f $@
	$(AM_V_AR)$(AR_FOR_BUILD) $(ARFLAGS) $@ $(igen_libigen_a_OBJECTS) $(igen_libigen_a_LIBADD)
	$(AM_V_at)$(RANLIB_FOR_BUILD) $@

%D%/igen$(EXEEXT): $(%C%_igen_OBJECTS) $(%C%_igen_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_igen_OBJECTS) $(%C%_igen_LDADD)

# igen is a build-time only tool.  Override the default rules for it.
%D%/%.o: %D%/%.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@

# Build some of the files in standalone mode for developers of igen itself.
%D%/%-main.o: %D%/%.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -DMAIN -c $< -o $@

%C%_filter_SOURCES =
%C%_filter_LDADD = %D%/filter-main.o %D%/libigen.a

%C%_gen_SOURCES =
%C%_gen_LDADD = %D%/gen-main.o %D%/libigen.a

%C%_ld_cache_SOURCES =
%C%_ld_cache_LDADD = %D%/ld-cache-main.o %D%/libigen.a

%C%_ld_decode_SOURCES =
%C%_ld_decode_LDADD = %D%/ld-decode-main.o %D%/libigen.a

%C%_ld_insn_SOURCES =
%C%_ld_insn_LDADD = %D%/ld-insn-main.o %D%/libigen.a

%C%_table_SOURCES =
%C%_table_LDADD = %D%/table-main.o %D%/libigen.a

%C%_IGEN_TOOLS = \
	$(IGEN) \
	%D%/filter \
	%D%/gen \
	%D%/ld-cache \
	%D%/ld-decode \
	%D%/ld-insn \
	%D%/table
EXTRA_PROGRAMS += $(%C%_IGEN_TOOLS)
MOSTLYCLEANFILES += $(%C%_IGEN_TOOLS) %D%/libigen.a
