## Process this file with automake to generate Makefile.in
##
#   Copyright (C) 1993-2024 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = dejagnu foreign no-dist subdir-objects
ACLOCAL_AMFLAGS = -Im4 -I.. -I../config

GNULIB_PARENT_DIR = ..
@am__include@ @am__quote@$(GNULIB_PARENT_DIR)/gnulib/Makefile.gnulib.inc@am__quote@

srccom = $(srcdir)/common
srcroot = $(srcdir)/..

SIM_PRIMARY_TARGET = @SIM_PRIMARY_TARGET@

## We don't set some of these vars here, but we need to define them so they may
## be used consistently in local.mk files we include below.
pkginclude_HEADERS =
check_PROGRAMS =
noinst_PROGRAMS =
EXTRA_LIBRARIES =
noinst_LIBRARIES =
EXTRA_PROGRAMS =
BUILT_SOURCES =

CLEANFILES =
DISTCLEANFILES =
MOSTLYCLEANFILES = core
## We build some objects ourselves directly that Automake doesn't track, so
## make sure all objects in subdirs get cleaned up.
MOSTLYCLEANFILES += $(SIM_ENABLED_ARCHES:%=%/*.o)

CONFIG_STATUS_DEPENDENCIES = $(srcroot)/bfd/development.sh

AM_CFLAGS = \
	$(WERROR_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS_$(subst -,_,$(@D))) \
	$(AM_CFLAGS_$(subst -,_,$(@D)_$(@F)))
AM_CPPFLAGS = \
	$(INCGNU) \
	-I$(srcroot) \
	-I$(srcroot)/include \
	-I../bfd \
	-I.. \
	-I$(@D) \
	-I$(srcdir)/$(@D) \
	$(SIM_HW_CFLAGS) \
	$(SIM_INLINE) \
	$(AM_CPPFLAGS_$(subst -,_,$(@D))) \
	$(AM_CPPFLAGS_$(subst -,_,$(@D)_$(@F)))

AM_CPPFLAGS_FOR_BUILD = \
	-I$(srcroot)/include \
	$(SIM_HW_CFLAGS) \
	$(SIM_INLINE)
COMPILE_FOR_BUILD = $(CC_FOR_BUILD) $(AM_CPPFLAGS_FOR_BUILD) $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(BUILD_WERROR_CFLAGS) $(BUILD_WARN_CFLAGS)
LINK_FOR_BUILD = $(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) -o $@

## Deps to add to the install-data-local target.
SIM_INSTALL_DATA_LOCAL_DEPS =
## Deps to add to the install-exec-local target.
SIM_INSTALL_EXEC_LOCAL_DEPS =
## Deps to add to the uninstall-local target.
SIM_UNINSTALL_LOCAL_DEPS =

# Generate target constants for newlib/libgloss from its source tree.
# This file is shipped with distributions so we build in the source dir.
# Use `make nltvals' to rebuild.
.PHONY: nltvals
nltvals:
	$(srccom)/gennltvals.py --cpp "$(CPP)"

if ENABLE_SIM
pkginclude_HEADERS += \
	$(srcroot)/include/sim/callback.h \
	$(srcroot)/include/sim/sim.h
endif

## Used for our custom rules to calculate the dependency output base name.
## This form aligns with Automake's default rules.
SIM_DEPBASE = $(@D)/$(DEPDIR)/$(@F:.o=)
## Actual full compile command like Automake's default rules.
SIM_COMPILE = \
	$(AM_V_CC)$(COMPILE) -MT $@ -MD -MP -MF $(SIM_DEPBASE).Tpo -c -o $@ $< && \
	$(am__mv) $(SIM_DEPBASE).Tpo $(SIM_DEPBASE).Po

include common/local.mk
include igen/local.mk
include testsuite/local.mk

## Arch includes must come after common/local.mk.
if SIM_ENABLE_ARCH_aarch64
include aarch64/local.mk
endif
if SIM_ENABLE_ARCH_arm
include arm/local.mk
endif
if SIM_ENABLE_ARCH_avr
include avr/local.mk
endif
if SIM_ENABLE_ARCH_bfin
include bfin/local.mk
endif
if SIM_ENABLE_ARCH_bpf
include bpf/local.mk
endif
if SIM_ENABLE_ARCH_cr16
include cr16/local.mk
endif
if SIM_ENABLE_ARCH_cris
include cris/local.mk
endif
if SIM_ENABLE_ARCH_d10v
include d10v/local.mk
endif
if SIM_ENABLE_ARCH_erc32
include erc32/local.mk
endif
if SIM_ENABLE_ARCH_examples
include example-synacor/local.mk
endif
if SIM_ENABLE_ARCH_frv
include frv/local.mk
endif
if SIM_ENABLE_ARCH_ft32
include ft32/local.mk
endif
if SIM_ENABLE_ARCH_h8300
include h8300/local.mk
endif
if SIM_ENABLE_ARCH_iq2000
include iq2000/local.mk
endif
if SIM_ENABLE_ARCH_lm32
include lm32/local.mk
endif
if SIM_ENABLE_ARCH_m32c
include m32c/local.mk
endif
if SIM_ENABLE_ARCH_m32r
include m32r/local.mk
endif
if SIM_ENABLE_ARCH_m68hc11
include m68hc11/local.mk
endif
if SIM_ENABLE_ARCH_mcore
include mcore/local.mk
endif
if SIM_ENABLE_ARCH_microblaze
include microblaze/local.mk
endif
if SIM_ENABLE_ARCH_mips
include mips/local.mk
endif
if SIM_ENABLE_ARCH_mn10300
include mn10300/local.mk
endif
if SIM_ENABLE_ARCH_moxie
include moxie/local.mk
endif
if SIM_ENABLE_ARCH_msp430
include msp430/local.mk
endif
if SIM_ENABLE_ARCH_or1k
include or1k/local.mk
endif
if SIM_ENABLE_ARCH_ppc
include ppc/local.mk
endif
if SIM_ENABLE_ARCH_pru
include pru/local.mk
endif
if SIM_ENABLE_ARCH_riscv
include riscv/local.mk
endif
if SIM_ENABLE_ARCH_rl78
include rl78/local.mk
endif
if SIM_ENABLE_ARCH_rx
include rx/local.mk
endif
if SIM_ENABLE_ARCH_sh
include sh/local.mk
endif
if SIM_ENABLE_ARCH_v850
include v850/local.mk
endif

install-data-local: installdirs $(SIM_INSTALL_DATA_LOCAL_DEPS)
	$(AM_V_at)$(MKDIR_P) $(DESTDIR)$(libdir)
	lib=`echo sim | sed '$(program_transform_name)'`; \
	for d in $(SIM_ENABLED_ARCHES); do \
		n="$$lib"; \
		[ "$(SIM_PRIMARY_TARGET)" = "$$d" ] || n="$$n-$$d"; \
		n="lib$$n.a"; \
		$(INSTALL_DATA) $$d/libsim.a $(DESTDIR)$(libdir)/$$n || exit 1; \
	done

install-exec-local: installdirs $(SIM_INSTALL_EXEC_LOCAL_DEPS)
	$(AM_V_at)$(MKDIR_P) $(DESTDIR)$(bindir)
	run=`echo run | sed '$(program_transform_name)'`; \
	for d in $(SIM_ENABLED_ARCHES); do \
		n="$$run"; \
		[ "$(SIM_PRIMARY_TARGET)" = "$$d" ] || n="$$n-$$d"; \
		$(LIBTOOL) --mode=install \
			$(INSTALL_PROGRAM) $$d/run$(EXEEXT) $(DESTDIR)$(bindir)/$$n$(EXEEXT) || exit 1; \
	done

uninstall-local: $(SIM_UNINSTALL_LOCAL_DEPS)
	rm -f $(DESTDIR)$(bindir)/run $(DESTDIR)$(libdir)/libsim.a
	for d in $(SIM_ENABLED_ARCHES); do \
		rm -f $(DESTDIR)$(bindir)/run-$$d $(DESTDIR)$(libdir)/libsim-$$d.a; \
	done
