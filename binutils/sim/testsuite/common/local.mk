## Copyright (C) 1997-2024 Free Software Foundation, Inc.
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

EXTRA_PROGRAMS += %D%/bits-gen

# We don't build this normally as it relies on the Berkeley SoftFloat/TestFloat
# projects being installed/available first.
EXTRA_PROGRAMS += %D%/fpu-tst

TESTS = \
	%D%/bits32m0 \
	%D%/bits32m31 \
	%D%/bits64m0 \
	%D%/bits64m63 \
	%D%/alu-tst
check_PROGRAMS += $(TESTS)

%C%_CPPFLAGS = \
	-I$(srcdir)/common \
	-I$(srcroot)/include \
	-I../bfd

# These tests are build-time only tools.  Override the default rules for them.
%D%/%.o: %D%/%.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) $(%C%_CPPFLAGS) -c $< -o $@

%D%/alu-tst$(EXEEXT): $(%C%_alu_tst_OBJECTS) $(%C%_alu_tst_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_alu_tst_OBJECTS) $(%C%_alu_tst_LDADD)

%D%/fpu-tst$(EXEEXT): $(%C%_fpu_tst_OBJECTS) $(%C%_fpu_tst_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_fpu_tst_OBJECTS) $(%C%_fpu_tst_LDADD)

%D%/bits-gen$(EXEEXT): $(%C%_bits_gen_OBJECTS) $(%C%_bits_gen_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_bits_gen_OBJECTS) $(%C%_bits_gen_LDADD)

%D%/bits32m0$(EXEEXT): $(%C%_bits32m0_OBJECTS) $(%C%_bits32m0_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_bits32m0_OBJECTS) $(%C%_bits32m0_LDADD)

%D%/bits32m0.c: %D%/bits-gen$(EXEEXT) %D%/bits-tst.c
	$(AM_V_GEN)$< 32 0 big > $@.tmp
	$(AM_V_at)cat $(srcdir)/%D%/bits-tst.c >> $@.tmp
	$(AM_V_at)mv $@.tmp $@

%D%/bits32m31$(EXEEXT): $(%C%_bits32m31_OBJECTS) $(%C%_bits32m31_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_bits32m31_OBJECTS) $(%C%_bits32m31_LDADD)

%D%/bits32m31.c: %D%/bits-gen$(EXEEXT) %D%/bits-tst.c
	$(AM_V_GEN)$< 32 31 little > $@.tmp
	$(AM_V_at)cat $(srcdir)/%D%/bits-tst.c >> $@.tmp
	$(AM_V_at)mv $@.tmp $@

%D%/bits64m0$(EXEEXT): $(%C%_bits64m0_OBJECTS) $(%C%_bits64m0_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_bits64m0_OBJECTS) $(%C%_bits64m0_LDADD)

%D%/bits64m0.c: %D%/bits-gen$(EXEEXT) %D%/bits-tst.c
	$(AM_V_GEN)$< 64 0 big > $@.tmp
	$(AM_V_at)cat $(srcdir)/%D%/bits-tst.c >> $@.tmp
	$(AM_V_at)mv $@.tmp $@

%D%/bits64m63$(EXEEXT): $(%C%_bits64m63_OBJECTS) $(%C%_bits64m63_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_bits64m63_OBJECTS) $(%C%_bits64m63_LDADD)

%D%/bits64m63.c: %D%/bits-gen$(EXEEXT) %D%/bits-tst.c
	$(AM_V_GEN)$< 64 63 little > $@.tmp
	$(AM_V_at)cat $(srcdir)/%D%/bits-tst.c >> $@.tmp
	$(AM_V_at)mv $@.tmp $@

CLEANFILES += \
	%D%/bits-gen \
	%D%/bits32m0.c \
	%D%/bits32m31.c \
	%D%/bits64m0.c \
	%D%/bits64m63.c
