## See sim/Makefile.am.
##
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

## Tweak the site.exp so it works with plain `runtest` from user.
EXTRA_DEJAGNU_SITE_CONFIG = site-sim-config.exp

# Custom verbose test variables that automake doesn't provide (yet?).
AM_V_RUNTEST = $(AM_V_RUNTEST_@AM_V@)
AM_V_RUNTEST_ = $(AM_V_RUNTEST_@AM_DEFAULT_V@)
AM_V_RUNTEST_0 =  @echo "  RUNTEST  $(RUNTESTFLAGS) $*";
AM_V_RUNTEST_1 =

site-sim-config.exp: Makefile
	$(AM_V_GEN)( \
	echo "set SIM_PRIMARY_TARGET \"$(SIM_PRIMARY_TARGET)\""; \
	echo "set builddir \"$(builddir)\""; \
	echo "set srcdir \"$(srcdir)/testsuite\""; \
	$(foreach V,$(SIM_TOOLCHAIN_VARS),echo "set $(V) \"$($(V))\"";) \
	) > $@

DO_RUNTEST = \
	LC_ALL=C; export LC_ALL; \
	EXPECT=${EXPECT} ; export EXPECT ; \
	runtest=$(RUNTEST); \
	$$runtest $(RUNTESTFLAGS)

# Ignore dirs that only contain configuration settings.
check/./config/%.exp: ; @true
check/config/%.exp: ; @true
check/./lib/%.exp: ; @true
check/lib/%.exp: ; @true

check/%.exp:
	$(AM_V_at)mkdir -p testsuite/$*
	$(AM_V_RUNTEST)$(DO_RUNTEST) --objdir testsuite/$* --outdir testsuite/$* $*.exp

check-DEJAGNU-parallel:
	$(AM_V_at)( \
	set -- `cd $(srcdir)/testsuite && find . -name '*.exp' -printf '%P\n' | sed 's:[.]exp$$::'`; \
	$(MAKE) -k `printf 'check/%s.exp ' $$@`; \
	ret=$$?; \
	set -- `printf 'testsuite/%s/ ' $$@`; \
	$(SHELL) $(srcroot)/contrib/dg-extract-results.sh \
	  `find $$@ -maxdepth 1 -name testrun.sum 2>/dev/null | sort` > testrun.sum; \
	$(SHELL) $(srcroot)/contrib/dg-extract-results.sh -L \
	  `find $$@ -maxdepth 1 -name testrun.log 2>/dev/null | sort` > testrun.log; \
	echo; \
	$(SED) -n '/^.*===.*Summary.*===/,$$p' testrun.sum; \
	exit $$ret)

check-DEJAGNU-single:
	$(AM_V_RUNTEST)$(DO_RUNTEST)

# If running a single job, invoking runtest once is faster & has nicer output.
check-DEJAGNU: site.exp
	$(AM_V_at)(set -e; \
	EXPECT=${EXPECT} ; export EXPECT ; \
	runtest=$(RUNTEST); \
	if $(SHELL) -c "$$runtest --version" > /dev/null 2>&1; then \
	  case "$(MAKEFLAGS)" in \
	  *-j*) $(MAKE) check-DEJAGNU-parallel;; \
	  *)    $(MAKE) check-DEJAGNU-single;; \
	  esac; \
	else \
	  echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi)

MOSTLYCLEANFILES += \
	site-sim-config.exp testrun.log testrun.sum

include %D%/common/local.mk
