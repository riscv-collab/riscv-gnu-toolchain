## See sim/Makefile.am
##
## Copyright (C) 1993-2024 Free Software Foundation, Inc.
## Written by Cygnus Support
## Modified by J.Gaisler ESA/ESTEC
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

READLINE_SRC = $(srcroot)/readline/readline
AM_CPPFLAGS_%C% = $(READLINE_CFLAGS)

## UARTS run at about 115200 baud (simulator time). Add -DFAST_UART to
## CFLAGS if faster (infinite) UART speed is desired. Might affect the
## behaviour of UART interrupt routines ...
AM_CPPFLAGS_%C% += -DFAST_UART

nodist_%C%_libsim_a_SOURCES = \
	%D%/modules.c
%C%_libsim_a_SOURCES = \
	$(common_libcommon_a_SOURCES)
%C%_libsim_a_LIBADD = \
	%D%/erc32.o \
	%D%/exec.o \
	%D%/float.o \
	%D%/func.o \
	%D%/help.o \
	%D%/interf.o
$(%C%_libsim_a_OBJECTS) $(%C%_libsim_a_LIBADD): %D%/hw-config.h

noinst_LIBRARIES += %D%/libsim.a

## Override wildcards that trigger common/modules.c to be (incorrectly) used.
%D%/modules.o: %D%/modules.c

%D%/%.o: common/%.c ; $(SIM_COMPILE)
-@am__include@ %D%/$(DEPDIR)/*.Po

%C%_run_SOURCES =
%C%_run_LDADD = \
	%D%/sis.o \
	%D%/libsim.a \
	$(SIM_COMMON_LIBS) $(READLINE_LIB) $(TERMCAP_LIB)

%D%/sis$(EXEEXT): %D%/run$(EXEEXT)
	$(AM_V_GEN)ln $< $@ 2>/dev/null || $(LN_S) $< $@ 2>/dev/null || cp -p $< $@

noinst_PROGRAMS += %D%/run %D%/sis

%C%docdir = $(docdir)/%C%
%C%doc_DATA = %D%/README.erc32 %D%/README.gdb %D%/README.sis

SIM_INSTALL_EXEC_LOCAL_DEPS += sim-%D-install-exec-local
sim-%D-install-exec-local: installdirs
	$(AM_V_at)$(MKDIR_P) $(DESTDIR)$(bindir)
	n=`echo sis | sed '$(program_transform_name)'`; \
	$(LIBTOOL) --mode=install $(INSTALL_PROGRAM) %D%/run$(EXEEXT) $(DESTDIR)$(bindir)/$$n$(EXEEXT)

SIM_UNINSTALL_LOCAL_DEPS += sim-%D%-uninstall-local
sim-%D%-uninstall-local:
	rm -f $(DESTDIR)$(bindir)/sis
